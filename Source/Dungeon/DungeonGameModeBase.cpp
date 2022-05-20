// Copyright Epic Games, Inc. All Rights Reserved.

#include "DungeonGameModeBase.h"

#include <Logic/game.h>

#include "DungeonMainWidget.h"
#include "Actor/DungeonPlayerController.h"
#include "Actor/MapCursorPawn.h"
#include "Algo/Accumulate.h"
#include "Algo/FindLast.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/DataTable.h"
#include "GameFramework/GameStateBase.h"
#include "Kismet/KismetSystemLibrary.h"

ADungeonGameModeBase::ADungeonGameModeBase(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
  static ConstructorHelpers::FClassFinder<UDungeonMainWidget> HandlerClass(TEXT("/Game/Blueprints/MainWidget"));
  MainWidgetClass = HandlerClass.Class;

  PrimaryActorTick.bCanEverTick = true;
  UnitTable = ObjectInitializer.CreateDefaultSubobject<UDataTable>(this, "Default");
  UnitTable->RowStruct = FDungeonLogicUnit::StaticStruct();
  Game = ObjectInitializer.CreateDefaultSubobject<UDungeonLogicGame>(this, "DefaultDebugMap");
  Game->Init();
}

void ADungeonGameModeBase::BeginPlay()
{
  Super::BeginPlay();

  UnitTable->GetAllRows(TEXT(""), unitsArray);
  TMap<int, FDungeonLogicUnit> loadedUnitTypes;
  for (auto unit : unitsArray)
  {
    loadedUnitTypes.Add(unit->id, *unit);
  }

  FString jsonStringTMX;
  if (FFileHelper::LoadFileToString(jsonStringTMX,
                                    ToCStr(FPaths::Combine(FPaths::ProjectDir(), TEXT("mapassignments.tmj")))))
  {
    TSharedPtr<FJsonObject> jsonTMX;
    TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<>::Create(jsonStringTMX);
    if (FJsonSerializer::Deserialize(JsonReader, jsonTMX))
    {
      FTiledMap TiledMap;
      FJsonObjectConverter::JsonObjectToUStruct<FTiledMap>(jsonTMX.ToSharedRef(), &TiledMap);

      for (int i = 0; i < TiledMap.layers[0].height; ++i)
        for (int j = 0; j < TiledMap.layers[0].width; ++j)
        {
          int dataIndex = i * TiledMap.layers[0].height + j;
          auto value = TiledMap.layers[0].data[dataIndex];
          if (value > 0 && loadedUnitTypes.Contains(value))
          {
            auto zaunit = loadedUnitTypes[value];
            zaunit.id = dataIndex;
            Game->map.loadedUnits.Add(zaunit.id, zaunit);

            FIntPoint positionPlacement{i, j};
            Game->map.unitAssignment.Add(positionPlacement, dataIndex);

            auto unitActor = GetWorld()->SpawnActor<ADungeonUnitActor>(UnitActorPrefab, FVector::ZeroVector,
                                                                       FRotator::ZeroRotator);
            Game->unitIdToActor.Add(zaunit.id, unitActor);
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

  // FSimpleTileGraph gridGraph = FSimpleTileGraph(this->Game->map);
  // FGraphAStar<FSimpleTileGraph> graph = FGraphAStar<FSimpleTileGraph>(gridGraph);
  // TArray<FIntPoint> outPath;
  // EGraphAStarResult result = graph.FindPath(FSimpleTileGraph::FNodeRef{ 0, 0 }, FSimpleTileGraph::FNodeRef{ 5, 3 }, gridGraph, outPath);
  // for (int i = 0, j = 1; j < outPath.Num(); ++i, ++j)
  // {
  // 	SubmitLinearAnimation(Game->unitIdToActor.FindChecked(1), outPath[i], outPath[j], 0.25);
  // }

  AActor* tileShowPrefab = GetWorld()->SpawnActor<AActor>(TileShowPrefab, FVector::ZeroVector, FRotator::ZeroRotator);
  tileVisualizationComponent = static_cast<UTileVisualizationComponent*>(
    tileShowPrefab->GetComponentByClass(UTileVisualizationComponent::StaticClass()));

  AMapCursorPawn* mapPawn = static_cast<AMapCursorPawn*>(GetWorld()->GetFirstPlayerController()->GetPawn());
  if (mapPawn != NULL)
  {
    // mapPawn->CursorEvent.AddUObject(this, &ADungeonGameModeBase::setCurrentPosition);
    // mapPawn->QueryInput.AddUObject(this, &ADungeonGameModeBase::setCurrentPosition);
    auto state = MakeShared<FSelectingGameState>(*this);
    currentState = state;
    state->Enter();

    MainWidget = CreateWidget<UDungeonMainWidget>(this->GetWorld()->GetFirstPlayerController(), MainWidgetClass);
    MainWidget->Wait->OnClicked.AddUniqueDynamic(this, &ADungeonGameModeBase::HandleWait);
    MainWidget->AddToViewport();

    this->setCurrentGameState(EGameState::Selecting);
  }
}

void ADungeonGameModeBase::Reduce()
{
  do
  {
    TSharedPtr<FAction> action;
    if (reactEventQueue.Dequeue(action))
    {
      ADungeonGameModeBase* wow = this;
      action->Apply(wow);
    }
    React();
  }
  while (!reactEventQueue.IsEmpty());
}

void ADungeonGameModeBase::React()
{
  AMapCursorPawn* mapPawn = static_cast<AMapCursorPawn*>(GetWorld()->GetFirstPlayerController()->GetPawn());
  APlayerController* controller = this->GetNetOwningPlayer()->GetPlayerController(this->GetWorld());
  const bool queryTriggered = mapPawn->ConsumeQueryCalled();

  FDungeonLogicMap& DungeonLogicMap = Game->map;
  TMap<FIntPoint, int>& unitAssignmentMap = DungeonLogicMap.unitAssignment;
  TSet<FIntPoint> moveLocations;
  auto foundId = unitAssignmentMap.Contains(CurrentPosition) ? unitAssignmentMap.FindChecked(CurrentPosition) : -1;
  auto foundUnit = DungeonLogicMap.loadedUnits.Find(foundId);
  bool unitCanTakeAction = foundUnit != nullptr ? foundUnit->state != ActionTaken : false;
  if (unitCanTakeAction)
  {
    if (CurrentGameState != SelectingTarget)
      lastSeenId = foundId;
    moveLocations = manhattanReachablePoints<TSet<FIntPoint>>(
      DungeonLogicMap.Width, DungeonLogicMap.Height, foundUnit->movement * 2, CurrentPosition);
    moveLocations.Remove(CurrentPosition);
  }

  ReactVisualization(moveLocations, CurrentGameState);
  for (auto LoadedUnit : DungeonLogicMap.loadedUnits)
  {
    TWeakObjectPtr<ADungeonUnitActor> DungeonUnitActor = Game->unitIdToActor.FindChecked(LoadedUnit.Key);
    DungeonUnitActor->React(LoadedUnit.Value);
    DungeonUnitActor->SetActorLocation(
      FVector(*DungeonLogicMap.unitAssignment.FindKey(LoadedUnit.Key)) * TILE_POINT_SCALE);
  }

  if (CurrentGameState == EGameState::SelectingAbility)
  {
    mapPawn->DisableInput(controller);
  }
  else
  {
    mapPawn->EnableInput(controller);
    if (queryTriggered)
    {
      if (CurrentGameState == EGameState::Selecting && unitCanTakeAction)
      {
        auto attackWidget = StaticCastSharedRef<SButton>(MainWidget->Attack->TakeWidget());
        attackWidget->SetOnClicked(FOnClicked::CreateLambda([this](int id)
        {
          FAttackResults results = AttackVisualization(id);
          AttackDelegate.Broadcast(results);
          Dispatch(FChangeState(EGameState::SelectingTarget));
          return FReply::Handled();
        }, foundUnit->id));
        Dispatch(FChangeState(EGameState::SelectingAbility));
      }
      else if (CurrentGameState == EGameState::SelectingTarget)
      {
        auto id = lastSeenId;
        FMoveAction moveAction(id, CurrentPosition);
        if (moveAction.canMove(this, lastMoveLocations))
        {
          Dispatch(MoveTemp(moveAction));
          Dispatch(FChangeState(EGameState::Selecting));
        }
      }
    }
  }

  bool visible = CurrentGameState == EGameState::SelectingAbility;
  if (visible)
  {
    MainWidget->MainMenu->SetVisibility(ESlateVisibility::Visible);
  }
  else
  {
    MainWidget->MainMenu->SetVisibility(ESlateVisibility::Collapsed);
  }

  RefocusMenu();
}

void FSelectingGameState::OnCursorPositionUpdate(FIntPoint pt)
{
  UWorld* InWorld = gameMode.GetWorld();
  APlayerController* controller = InWorld->GetFirstPlayerController();
  AMapCursorPawn* mapPawn = Cast<AMapCursorPawn>(controller->GetPawn());
  auto& CurrentPosition = pt;

  FDungeonLogicMap& DungeonLogicMap = gameMode.Game->map;
  TMap<FIntPoint, int>& unitAssignmentMap = DungeonLogicMap.unitAssignment;
  TSet<FIntPoint> moveLocations;
  auto foundId = unitAssignmentMap.Contains(CurrentPosition) ? unitAssignmentMap.FindChecked(CurrentPosition) : -1;
  foundUnit = DungeonLogicMap.loadedUnits.Find(foundId);
  bool unitCanTakeAction = foundUnit != nullptr ? foundUnit->state != ActionTaken : false;
  if (unitCanTakeAction)
  {
    moveLocations = manhattanReachablePoints<TSet<FIntPoint>>(
      DungeonLogicMap.Width, DungeonLogicMap.Height, foundUnit->movement * 2, CurrentPosition);
    moveLocations.Remove(CurrentPosition);
  }

  gameMode.tileVisualizationComponent->Clear();
  gameMode.tileVisualizationComponent->ShowTiles(moveLocations);
  this->targets.Add(ETargetsAvailableId::move, moveLocations);
}

void FSelectingGameState::OnCursorQuery(FIntPoint)
{
  if (foundUnit != nullptr)
  {
    if (foundUnit->state == Free)
    {
      auto attackWidget = StaticCastSharedRef<SButton>(gameMode.MainWidget->Attack->TakeWidget());
      attackWidget->SetOnClicked(FOnClicked::CreateLambda([this](int id)
      {
        auto results = gameMode.AttackVisualization(id);
        gameMode.AttackDelegate.Broadcast(results);
        gameMode.Dispatch(FChangeState(EGameState::SelectingTarget));
        return FReply::Handled();
      }, foundUnit->id));
      gameMode.currentState = gameMode.MakeChangeState<FSelectingMenu>(this, gameMode, foundUnit,
                                                                       MoveTemp(this->targets));
    }
  }
}

void FSelectingGameState::Enter()
{
  AMapCursorPawn* MapCursorPawn = gameMode.GetMapCursorPawn();
  //TODO: Hmm making the assumption that we're the set state? HMM
  handles.Push(MapCursorPawn->QueryInput.AddRaw(this, &FSelectingGameState::OnCursorQuery));
  handles.Push(MapCursorPawn->CursorEvent.AddRaw(this, &FSelectingGameState::OnCursorPositionUpdate));
}

void FSelectingGameState::Exit()
{
  AMapCursorPawn* MapCursorPawn = gameMode.GetMapCursorPawn();
  MapCursorPawn->QueryInput.Remove(handles[0]);
  MapCursorPawn->CursorEvent.Remove(handles[1]);
}

void FSelectingTargetGameState::OnCursorQuery(FIntPoint pt)
{
  FMoveAction moveAction(this->instigator.id, pt);
  if (moveAction.canMove(&GameModeBase, this->TilesExtent))
  {
    GameModeBase.Dispatch(MoveTemp(moveAction));
    GameModeBase.Reduce();
    GameModeBase.currentState = GameModeBase.MakeChangeState<FSelectingGameState>(this, GameModeBase);
  }
}

void ADungeonGameModeBase::HandleMove()
{
  this->setCurrentGameState(EGameState::SelectingTarget);
}

void ADungeonGameModeBase::HandleWait()
{
  this->setCurrentGameState(EGameState::Selecting);
}

void ADungeonGameModeBase::ReactVisualization(TSet<FIntPoint> moveLocations, EGameState PassedGameState)
{
  if (PassedGameState == SelectingTarget)
    return;
  this->lastMoveLocations = moveLocations;
  tileVisualizationComponent->Clear();
  tileVisualizationComponent->ShowTiles(moveLocations);
}

void ADungeonGameModeBase::ReactMenu(bool visible)
{
  MainWidget->MainMenu->SetVisibility(visible ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
  RefocusMenu();
}

void ADungeonGameModeBase::RefocusMenu()
{
  if (MainWidget->Move->HasUserFocus(0))
    return;
  MainWidget->Move->SetFocus();
}

FAttackResults ADungeonGameModeBase::AttackVisualization(int attackingUnitId)
{
  FDungeonLogicMap DungeonLogicMap = Game->map;
  auto attackingUnitPosition = DungeonLogicMap.unitAssignment.FindKey(attackingUnitId);
  FDungeonLogicUnit attackingUnit = DungeonLogicMap.loadedUnits.FindChecked(attackingUnitId);
  FAttackResults results;

  results.AllAttackTiles = DungeonLogicMap.constrainedManhattanReachablePoints<TSet<FIntPoint>>(
    attackingUnit.getAttackRange(), *attackingUnitPosition);
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

void ADungeonGameModeBase::Tick(float deltaTime)
{
  Super::Tick(deltaTime);

  if (this->AnimationQueue.IsEmpty())
    return;

  (*this->AnimationQueue.Peek())->TickTimeline(deltaTime);
}
