// Copyright Epic Games, Inc. All Rights Reserved.

// #pragma push_macro("check")
// #undef check

#include "DungeonGameModeBase.h"

#include <Logic/DungeonGameState.h>

#include "Actions/EndTurnAction.h"
#include "Actor/DungeonPlayerController.h"
#include "Actor/MapCursorPawn.h"
#include "Algo/Accumulate.h"
#include "Algo/FindLast.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/DataTable.h"
#include "GameFramework/GameStateBase.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Lenses/model.hpp"
#include "Logic/SimpleTileGraph.h"
#include "Serialization/Csv/CsvParser.h"
#include "SingleSubmitHandler.h"
#include "State/SelectingGameState.h"
#include "immer/map.hpp"
#include "lager/event_loop/manual.hpp"
#include "lager/lenses/tuple.hpp"
#include "lager/store.hpp"
#include "lager/util.hpp"

ADungeonGameModeBase::ADungeonGameModeBase(const FObjectInitializer& ObjectInitializer) :
  Super(ObjectInitializer)
{
  static ConstructorHelpers::FClassFinder<UDungeonMainWidget> HandlerClass(TEXT("/Game/Blueprints/Widgets/MainWidget"));
  MainWidgetClass = HandlerClass.Class;

  PrimaryActorTick.bCanEverTick = true;
  UnitTable = ObjectInitializer.CreateDefaultSubobject<UDataTable>(this, "Default");
  UnitTable->RowStruct = FDungeonLogicUnit::StaticStruct();
}

template <typename InnerReducer,
          typename WrappedModel,
          typename WrappedAction>
auto update_history(InnerReducer&& innerReducer,
                    FHistoryModel<WrappedModel> m,
                    THistoryAction<WrappedAction> a)
-> FHistoryModel<WrappedModel>
{
  return Visit(lager::visitor{
                 [&](undo_action a)
                 {
                   m.stateStack.Pop();
                   return m;
                   // return update_history(r, m, goto_action{m.position - 1});
                 },
                 // [&] (redo_action a) {
                 //     return update_history(r, m, goto_action{m.position + 1});
                 // },
                 // [&] (goto_action a) {
                 //     if (a.position >= 0 && a.position < m.history.size())
                 //         m.position = a.position;
                 //     return m;
                 // },
                 [&](WrappedAction action)
                 {
                   auto updatedModel = innerReducer(m, action);
                   if (&updatedModel != &(m.stateStack.Top()))
                   {
                     m.stateStack.Push(updatedModel);
                   }
                   return m;
                 },
               }, a);
}

auto WorldStateReducer =
  [](FDungeonWorldState Model, TAction worldAction)
{
  return Visit(lager::visitor
               {
                 [&](FBackAction)
                 {
                   return Visit(lager::visitor{
                                  [&](auto all)
                                  {
                                    Model.InteractionContext.Set<FMainMenu>(FMainMenu());
                                    return Model;
                                  },
                                  [&](FMainMenu)
                                  {
                                    Model.InteractionContext.Set<FSelectingUnit>(FSelectingUnit());
                                    return Model;
                                  }
                                }, Model.InteractionContext);
                 },
                 [&](FEndTurnAction action)
                 {
                   const int MAX_PLAYERS = 2;
                   const auto nextTeamId = (Model.TurnState.teamId % MAX_PLAYERS) + 1;

                   Model.TurnState = {nextTeamId};
                   return Model;
                 },
                 [&](auto&& action)
                 {
                   using T = typename TDecay<decltype(action)>::Type;
                   if constexpr (TIsInTypeUnion<T, FCombatAction, FMoveAction, FWaitAction>::Value)
                   {
                     FDungeonLogicUnit& foundUnit = Model.map.loadedUnits.FindChecked(action.InitiatorId);
                     foundUnit.state = UnitState::ActionTaken;
                     Model.TurnState.unitsFinished.Add(foundUnit.Id);

                     lager::visitor{
                       [&](FMoveAction MoveAction)
                       {
                         FDungeonLogicMap& DungeonLogicMap = Model.map;
                         DungeonLogicMap.unitAssignment.Remove(
                           *DungeonLogicMap.unitAssignment.FindKey(MoveAction.InitiatorId));
                         DungeonLogicMap.unitAssignment.
                                         Add(MoveAction.Destination, MoveAction.InitiatorId);
                       },
                       [&](FCombatAction CombatAction)
                       {
                         FDungeonLogicUnit& Unit = CombatAction.updatedUnit;
                         Unit.state = Model.TurnState.unitsFinished.Contains(Unit.Id)
                                        ? UnitState::ActionTaken
                                        : UnitState::Free;
                       },
                       [&](FWaitAction WaitAction)
                       {
                       }
                     }(action);
                   }
                   
                   Model.InteractionContext.Set<FSelectingUnit>(FSelectingUnit{});
                   return Model;
                 },
               }, worldAction);
};

auto with_history = [](auto next)
{
  return [next](
    auto action,
    auto&& model,
    auto&& reducer,
    auto&& loop,
    auto&& deps,
    auto&& tags)
  {
    return next(action,
                model,
                [reducer](auto m, auto a) { return update_history(reducer, m, a); },
                LAGER_FWD(loop),
                LAGER_FWD(deps),
                LAGER_FWD(tags));
  };
};

void ADungeonGameModeBase::BeginPlay()
{
  Super::BeginPlay();

  TArray<FDungeonLogicUnitRow*> unitsArray;
  UnitTable->GetAllRows(TEXT(""), unitsArray);
  TMap<int, FDungeonLogicUnitRow> loadedUnitTypes;
  for (auto unit : unitsArray)
  {
    loadedUnitTypes.Add(unit->unitData.Id, *unit);
  }

  FString StuffR;
  FString TheStr = FPaths::Combine(FPaths::ProjectDir(), TEXT("theBoard.csv"));
  if (FFileHelper::LoadFileToString(StuffR, ToCStr(TheStr)))
  {
    const FCsvParser Parser(MoveTemp(StuffR));
    const auto& Rows = Parser.GetRows();
    int32 height = Rows.Num();

    Game.map.loadedTiles.Add(1, {1, "Grass", 1});

    for (int i = 0; i < height; i++)
    {
      int32 width = Rows[i].Num();
      Game.map.Height = height;
      Game.map.Width = width;
      Game.TurnState.teamId = 1;
      for (int j = 0; j < width; j++)
      {
        int dataIndex = i * width + j;
        auto value = FCString::Atoi(Rows[i][j]);

        Game.map.tileAssignment.Add({i, j}, 1);

        if (loadedUnitTypes.Contains(value))
        {
          auto zaRow = loadedUnitTypes[value];
          auto zaunit = zaRow.unitData;
          zaunit.Id = dataIndex;
          zaunit.Name = FString::FormatAsNumber(dataIndex);
          Game.map.loadedUnits.Add(zaunit.Id, zaunit);

          FIntPoint positionPlacement{i, j};
          Game.map.unitAssignment.Add(positionPlacement, dataIndex);

          UClass* UnrealActor = zaRow.UnrealActor.Get();
          FTransform Transform = FTransform{FRotator::ZeroRotator, FVector::ZeroVector};
          auto unitActor = GetWorld()->SpawnActor<ADungeonUnitActor>(UnrealActor, Transform);
          unitActor->id = zaunit.Id;

          Game.unitIdToActor.Add(zaunit.Id, unitActor);
          unitActor->SetActorLocation(TilePositionToWorldPoint(positionPlacement));
        }
      }
    }
  }

  store = MakeUnique<lager::store<THistoryAction<TAction>, FHistoryModel<FDungeonWorldState>>>(
    lager::make_store<THistoryAction<TAction>>(
      FHistoryModel<FDungeonWorldState>(FDungeonWorldState(this->Game)),
      lager::with_manual_event_loop{},
      lager::with_reducer(WorldStateReducer),
      with_history
    ));

  for (auto UnitIdToActor : static_cast<const FDungeonWorldState>(store->get()).unitIdToActor)
  {
    UnitIdToActor.Value->hookIntoStore();
  }

  FString Result;
  FString Str = FPaths::Combine(FPaths::ProjectDir(), TEXT("UnitIds.csv"));
  if (FFileHelper::LoadFileToString(Result, ToCStr(Str)))
  {
    UDataTable* DataTable = NewObject<UDataTable>();
    DataTable->RowStruct = FStructThing::StaticStruct();
    DataTable->CreateTableFromCSVString(Result);
    TArray<FStructThing*> wow;
    DataTable->GetAllRows<FStructThing>(TEXT("wew"), wow);
    FStructThing* StructThing = DataTable->FindRow<FStructThing>(FName(TEXT("1")), TEXT("wow"));
    FString TableAsCSV = DataTable->GetTableAsCSV();
    FFileHelper::SaveStringToFile(TableAsCSV, ToCStr(FPaths::Combine(FPaths::ProjectDir(), TEXT("UnitIdsOutput.csv"))));
  }

  FActorSpawnParameters params;
  params.Name = FName("BoardVisualizationActor");
  auto tileShowPrefab = GetWorld()->SpawnActor<ATileVisualizationActor>(TileShowPrefab, FVector::ZeroVector,
                                                                        FRotator::ZeroRotator, params);

  MovementVisualization = tileShowPrefab->MovementVisualizationComponent;
  TileVisualizationComponent = tileShowPrefab->TileVisualizationComponent;

  CombatActionEvent.AddDynamic(this, &ADungeonGameModeBase::ApplyAction);

  MainWidget = CreateWidget<UDungeonMainWidget>(this->GetWorld()->GetFirstPlayerController(), MainWidgetClass);
  MainWidget->AddToViewport();

  auto InAttribute = FCoreStyle::GetDefaultFontStyle("Bold", 30);
  InAttribute.OutlineSettings.OutlineSize = 3.0;
  style = FCoreStyle::Get().GetWidgetStyle<FTextBlockStyle>("NormalText");
  style.SetColorAndOpacity(FSlateColor(FLinearColor::White));

  auto styleSetter = [&](FString&& fieldName, const auto& methodPointer) -> decltype(SVerticalBox::Slot())
  {
    return SVerticalBox::Slot().AutoHeight()[
      SNew(SHorizontalBox)
      + SHorizontalBox::Slot()
      [
        SNew(STextBlock)
           .TextStyle(&style)
           .Font(InAttribute)
           .Text(FText::FromString(fieldName))
      ]
      + SHorizontalBox::Slot()
      [
        SNew(STextBlock)
           .TextStyle(&style)
           .Font(InAttribute)
           .Text_UObject(this, methodPointer)
      ]
    ];
  };

  MainWidget->UnitDisplay->SetContent(
    SNew(SVerticalBox)
    + styleSetter("Name", &ADungeonGameModeBase::GetLastSeenUnitUnderCursorName)
    + styleSetter("HitPoints", &ADungeonGameModeBase::GetLastSeenUnitUnderCursorHitPoints)
    + styleSetter("Movement", &ADungeonGameModeBase::GetLastSeenUnitUnderCursorMovement)
    + styleSetter("Turn ID", &ADungeonGameModeBase::GetCurrentTurnId)
  );

  baseState = MakeShared<FSelectingGameState>(*this);
  baseState->Enter();
}

void ADungeonGameModeBase::GoBackOnInputState()
{
  if (stateStack.IsEmpty())
    return;
  auto popped = stateStack.Pop();
  popped->Exit();

  if (stateStack.IsEmpty())
    baseState->Enter();
  else
    stateStack.Top()->Enter();
}

TWeakPtr<FState> ADungeonGameModeBase::GetCurrentState()
{
  return stateStack.IsEmpty() ? baseState : stateStack.Top();
}

void ADungeonGameModeBase::CommitAndGotoBaseState()
{
  GetCurrentState().Pin()->Exit();
  stateStack.Empty();
  GetCurrentState().Pin()->Enter();
}

void ADungeonGameModeBase::React()
{
  // FDungeonLogicMap& DungeonLogicMap = Game.map;
  // for (auto LoadedUnit : DungeonLogicMap.loadedUnits)
  // {
  //   TWeakObjectPtr<ADungeonUnitActor> DungeonUnitActor = lager::view(
  //       attr(&ADungeonGameModeBase::Game)
  //       | attr(&FDungeonWorldState::unitIdToActor)
  //       | Find(LoadedUnit.Key), *this)
  //     .value();
  //   DungeonUnitActor->React(LoadedUnit.Value);
  //   DungeonUnitActor->SetActorLocation(
  //     FVector(*DungeonLogicMap.unitAssignment.FindKey(LoadedUnit.Key)) * TILE_POINT_SCALE);
  // }
}

void ADungeonGameModeBase::ApplyAction(FCombatAction action)
{
  Game.map.loadedUnits.Add(action.updatedUnit.Id, action.updatedUnit);
  FDungeonLogicUnit DungeonLogicUnit = Game.map.loadedUnits.FindChecked(action.InitiatorId);
  DungeonLogicUnit.state = UnitState::ActionTaken;
  UpdateUnitActor(action.updatedUnit);
  UpdateUnitActor(DungeonLogicUnit);
}

void ADungeonGameModeBase::RefocusMenu()
{
  if (MainWidget->Move->HasUserFocus(0))
    return;
  MainWidget->Move->SetFocus();
}

AMapCursorPawn* ADungeonGameModeBase::GetMapCursorPawn()
{
  return this
         ->GetWorld()
         ->GetFirstPlayerController()
         ->GetPawn<AMapCursorPawn>();
}

bool ADungeonGameModeBase::canUnitMoveToPointInRange(int unitId, FIntPoint destination,
                                                     const TSet<FIntPoint>& movementExtent)
{
  auto& map = Game.map;
  if (map.unitAssignment.Contains(destination))
    return false;

  if (map.unitAssignment.FindKey(unitId) == nullptr)
    return false;

  return movementExtent.Contains(destination) && Game.unitIdToActor.Contains(unitId);
}

TOptional<FDungeonLogicUnit> ADungeonGameModeBase::FindUnit(FIntPoint pt)
{
  auto& DungeonLogicMap = lager::view(mapLens, *this);
  if (DungeonLogicMap.unitAssignment.Contains(pt))
  {
    return DungeonLogicMap.loadedUnits[DungeonLogicMap.unitAssignment[pt]];
  }
  return TOptional<FDungeonLogicUnit>();
}

void ADungeonGameModeBase::Tick(float deltaTime)
{
  Super::Tick(deltaTime);

  if (this->AnimationQueue.IsEmpty())
    return;

  (*this->AnimationQueue.Peek())->TickTimeline(deltaTime);
}

void ADungeonGameModeBase::UpdateUnitActor(const FDungeonLogicUnit& unit)
{
  FDungeonLogicMap& DungeonLogicMap = Game.map;
  TWeakObjectPtr<ADungeonUnitActor> DungeonUnitActor = Game.unitIdToActor.FindChecked(unit.Id);
  DungeonUnitActor->React(unit);
  DungeonUnitActor->SetActorLocation(
    FVector(*DungeonLogicMap.unitAssignment.FindKey(unit.Id)) * TILE_POINT_SCALE);
}

void ADungeonGameModeBase::Dispatch(TAction unionAction)
{
  store->dispatch(TStoreAction(TInPlaceType<TAction>{}, unionAction));
}

FText ADungeonGameModeBase::GetCurrentTurnId() const
{
  return FCoerceToFText::Value(lager::view(turnStateLens, *this).teamId);
}

// #pragma pop_macro("check")
