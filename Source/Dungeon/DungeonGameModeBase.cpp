// Copyright Epic Games, Inc. All Rights Reserved.

#include "DungeonGameModeBase.h"

#include <Logic/DungeonGameState.h>

#include "DungeonMainWidget.h"
#include "SingleSubmitHandler.h"
#include "Actions/EndTurnAction.h"
#include "Actor/DungeonPlayerController.h"
#include "Actor/MapCursorPawn.h"
#include "Algo/Accumulate.h"
#include "Algo/FindLast.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/DataTable.h"
#include "GameFramework/GameStateBase.h"
#include "immer/map.hpp"
#include "Kismet/KismetSystemLibrary.h"
#include "lager/store.hpp"
#include "lager/util.hpp"
#include "lager/event_loop/manual.hpp"
#include "lager/lenses/tuple.hpp"
#include "Logic/SimpleTileGraph.h"
#include "Serialization/Csv/CsvParser.h"
#include "State/SelectingGameState.h"

ADungeonGameModeBase::ADungeonGameModeBase(const FObjectInitializer& ObjectInitializer) :
  Super(ObjectInitializer)
{
  static ConstructorHelpers::FClassFinder<UDungeonMainWidget> HandlerClass(TEXT("/Game/Blueprints/Widgets/MainWidget"));
  MainWidgetClass = HandlerClass.Class;

  PrimaryActorTick.bCanEverTick = true;
  UnitTable = ObjectInitializer.CreateDefaultSubobject<UDataTable>(this, "Default");
  UnitTable->RowStruct = FDungeonLogicUnit::StaticStruct();
}

auto WorldStateReducer =
  [](FDungeonWorldState model, TDungeonAction worldAction)
{
  return Visit(lager::visitor{
                 [&](FBackAction)
                 {
                   return Visit(lager::visitor{
                                  [&](auto all)
                                  {
                                    model.InteractionContext.Set<FMainMenu>(FMainMenu());
                                    return std::make_pair(model, lager::noop);
                                  },
                                  [&](FMainMenu)
                                  {
                                    model.InteractionContext.Set<FSelectingUnit>(FSelectingUnit());
                                    return std::make_pair(model, lager::noop);
                                  }
                                }, model.InteractionContext);
                 },
                 [&](FEmptyVariantState)
                 {
                   return std::make_pair(model, lager::noop);
                 }
               }, worldAction);
};

void ADungeonGameModeBase::BeginPlay()
{
  Super::BeginPlay();

  store = MakeUnique<lager::store<TDungeonAction, FDungeonWorldState>>(
    lager::make_store<TDungeonAction>(
      FDungeonWorldState(this->Game),
      lager::with_manual_event_loop{},
      lager::with_reducer(WorldStateReducer)));

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
          auto unitActor = GetWorld()->SpawnActor<ADungeonUnitActor>(UnrealActor, FVector::ZeroVector,
                                                                     FRotator::ZeroRotator);
          Game.unitIdToActor.Add(zaunit.Id, unitActor);
          unitActor->SetActorLocation(TilePositionToWorldPoint(positionPlacement));
        }
      }
    }
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

  AMapCursorPawn* mapPawn = static_cast<AMapCursorPawn*>(GetWorld()->GetFirstPlayerController()->GetPawn());
  if (mapPawn != NULL)
  {
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

  Dispatch(TAction(TInPlaceType<FEndTurnAction>{}));
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
  FDungeonLogicMap& DungeonLogicMap = Game.map;
  for (auto LoadedUnit : DungeonLogicMap.loadedUnits)
  {
    TWeakObjectPtr<ADungeonUnitActor> DungeonUnitActor = lager::view(
        attr(&ADungeonGameModeBase::Game)
        | attr(&FDungeonWorldState::unitIdToActor)
        | Find(LoadedUnit.Key), *this)
      .value();
    DungeonUnitActor->React(LoadedUnit.Value);
    DungeonUnitActor->SetActorLocation(
      FVector(*DungeonLogicMap.unitAssignment.FindKey(LoadedUnit.Key)) * TILE_POINT_SCALE);
  }
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
  Visit(lager::visitor{
          [&](FEndTurnAction action)
          {
            const int MAX_PLAYERS = 2;
            const auto nextTeamId = (this->Game.TurnState.teamId % MAX_PLAYERS) + 1;

            Game.TurnState = {nextTeamId};
          },
          [&](auto action)
          {
            using T = decltype(action);
            if constexpr (
              TOr<
                TIsSame<T, FCombatAction>,
                TIsSame<T, FMoveAction>,
                TIsSame<T, FWaitAction>
              >::Value)
            {
              FDungeonLogicUnit foundUnit = Game.map.loadedUnits.FindChecked(action.InitiatorId);
              foundUnit.state = UnitState::ActionTaken;
              Game.TurnState.unitsFinished.Add(foundUnit.Id);

              Visit(lager::visitor{
                      [&](FMoveAction MoveAction)
                      {
                        FDungeonLogicMap& DungeonLogicMap = Game.map;
                        DungeonLogicMap.unitAssignment.Remove(
                          *DungeonLogicMap.unitAssignment.FindKey(MoveAction.InitiatorId));
                        DungeonLogicMap.unitAssignment.Add(MoveAction.Destination, MoveAction.InitiatorId);

                        UpdateUnitActor(foundUnit);
                        CommitAndGotoBaseState();
                      },
                      [&](FCombatAction CombatAction)
                      {
                        CombatActionEvent.Broadcast(CombatAction);

                        FDungeonLogicUnit& Unit = CombatAction.updatedUnit;
                        Unit.state = Game.TurnState.unitsFinished.Contains(Unit.Id)
                                       ? UnitState::ActionTaken
                                       : UnitState::Free;
                        UpdateUnitActor(Unit);
                        UpdateUnitActor(foundUnit);
                        CommitAndGotoBaseState();
                      },
                      [&](FWaitAction action)
                      {
                        UpdateUnitActor(foundUnit);
                        CommitAndGotoBaseState();
                      },
                      [&](auto)
                      {
                      }
                    }, unionAction);
            }
          }
        }, unionAction);
}

FText ADungeonGameModeBase::GetCurrentTurnId() const
{
  return FCoerceToFText::Value(lager::view(turnStateLens, *this).teamId);
}
