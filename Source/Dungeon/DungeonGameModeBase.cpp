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
      Game.turnState.teamId = 1;
      for (int j = 0; j < width; j++)
      {
        int dataIndex = i * width + j;
        auto value = FCString::Atoi(Rows[i][j]);

        Game.map.tileAssignment.Add({i,j}, 1);
        
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
  auto tileShowPrefab = GetWorld()->SpawnActor<ATileVisualizationActor>(TileShowPrefab, FVector::ZeroVector, FRotator::ZeroRotator, params);
  
  // //TInlineComponentArray<UTileVisualizationComponent*> out = TInlineComponentArray<UTileVisualizationComponent*>(this, false);
  // TInlineComponentArray<UTileVisualizationComponent*> out;
  // tileShowPrefab->GetComponents(out);
  // MovementVisualization = *out.FindByPredicate([&](decltype(out)::ElementType& wow) {return wow->GetName().Equals("PathVisualization");});
  //
  // tileVisualizationComponent = static_cast<UTileVisualizationComponent*>(
  //   tileShowPrefab->GetComponentByClass(UTileVisualizationComponent::StaticClass()));
  
  MovementVisualization = tileShowPrefab->MovementVisualizationComponent;
  TileVisualizationComponent = tileShowPrefab->TileVisualizationComponent;

  CombatActionEvent.AddDynamic(this, &ADungeonGameModeBase::ApplyAction);

  // for (int i = 0, j = 1; j < outPath.Num(); ++i, ++j)
  // {
  // 	SubmitLinearAnimation(Game->unitIdToActor.FindChecked(1), outPath[i], outPath[j], 0.25);
  // }

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

    // auto newOne =
    //   lager::lenses::attr(&ADungeonGameModeBase::LastSeenUnitUnderCursor) 
    //   | lager::lenses::deref
    //   | lager::lenses::attr(&FDungeonLogicUnit::Name);

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

  Reduce(TAction(TInPlaceType<FEndTurnAction>{}));
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
        | attr(&FDungeonLogicGameState::unitIdToActor)
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

FAttackResults ADungeonGameModeBase::AttackVisualization(int attackingUnitId)
{
  FDungeonLogicMap DungeonLogicMap = Game.map;
  auto attackingUnitPosition = DungeonLogicMap.unitAssignment.FindKey(attackingUnitId);
  FDungeonLogicUnit attackingUnit = DungeonLogicMap.loadedUnits.FindChecked(attackingUnitId);
  FAttackResults results;

  results.AllAttackTiles =
    manhattanReachablePoints(
      DungeonLogicMap.Width, DungeonLogicMap.Height, attackingUnit.attackRange, *attackingUnitPosition);
  for (FIntPoint Point : results.AllAttackTiles)
  {
    if (DungeonLogicMap.unitAssignment.Contains(Point))
    {
      int unitId = DungeonLogicMap.unitAssignment[Point];
      FDungeonLogicUnit unitAtTile = DungeonLogicMap.loadedUnits[unitId];
      //TODO: Logic for determining friend foe?
      if (unitAtTile.teamId != attackingUnit.teamId)
      {
        results.AttackableUnitTiles.Add(Point);
      }
    }
  }
  return results;
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

namespace omar
{
  struct ActionA
  {
    int wow;
  };

  struct Model2
  {
    int wow;
  };

  struct Model
  {
    int age;
    Model2 thing;
  };

  using Action = std::variant<ActionA>;

  auto update(Model m, Action a)
  {
    return m;
  }
}

void ADungeonGameModeBase::Reduce(TAction unionAction)
{
  // omar::Model Model = omar::Model{1234, {4321} };
  //
  // lager::store<omar::Action, omar::Model> store = lager::make_store<omar::Action>(
  //   Model, lager::with_manual_event_loop{});
  //
  // auto model2Cursor = store.zoom(
  //   attr(&omar::Model::thing)
  //   | attr(&omar::Model2::wow))
  // .make();
  //
  // model2Cursor.watch([&](auto ey) {
  //   UKismetSystemLibrary::PrintString(this, FString::FromInt(ey) );
  // });
  //
  // lager::watch(store, [&](omar::Model wow)
  // {
  //   UKismetSystemLibrary::PrintString(this, "Check this shit out bro");
  // });
  //
  // store.dispatch(omar::Action(omar::ActionA{2}));

  Visit(lager::visitor{
          [&](FEndTurnAction action)
          {
            const int MAX_PLAYERS = 2;
            const auto nextTeamId = (this->Game.turnState.teamId % MAX_PLAYERS) + 1;

            Game.turnState = {nextTeamId};
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
              Game.turnState.unitsFinished.Add(foundUnit.Id);

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
                        Unit.state = Game.turnState.unitsFinished.Contains(Unit.Id)
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
                      [&](auto action)
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
