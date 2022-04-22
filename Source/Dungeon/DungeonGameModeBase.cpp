// Copyright Epic Games, Inc. All Rights Reserved.


#include "DungeonGameModeBase.h"

#include <Data/actions.h>
#include <Logic/game.h>

#include "DungeonUserWidget.h"
#include "LocalizationTargetTypes.h"
#include "Actor/DungeonPlayerController.h"
#include "Actor/MapCursorPawn.h"
#include "Algo/FindLast.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "GameFramework/GameStateBase.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Widgets/Layout/SConstraintCanvas.h"

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

ADungeonGameModeBase::ADungeonGameModeBase(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
  PrimaryActorTick.bCanEverTick = true;
  Game = CreateDefaultSubobject<UDungeonLogicGame>("own");
  Game->Init();
}

int* lastSeenId;

void ADungeonGameModeBase::React()
{
  AMapCursorPawn* mapPawn = static_cast<AMapCursorPawn*>(GetWorld()->GetFirstPlayerController()->GetPawn());
  APlayerController* controller = this->GetNetOwningPlayer()->GetPlayerController(this->GetWorld());
  const bool queryTriggered = mapPawn->ConsumeQueryCalled();

  FDungeonLogicMap& DungeonLogicMap = Game->map;
  TMap<FIntPoint, int>& unitAssignmentMap = DungeonLogicMap.unitAssignment;
  TSet<FIntPoint> moveLocations;
  auto foundId = unitAssignmentMap.Find(CurrentPosition);
  bool unitCanTakeAction = foundId != NULL && DungeonLogicMap.loadedUnits[*foundId].state != ActionTaken;
  if (unitCanTakeAction)
  {
    if (CurrentGameState != SelectingTarget)
      lastSeenId = foundId;
    auto& foundUnit = DungeonLogicMap.loadedUnits.FindChecked(*foundId);
    moveLocations = manhattanReachablePoints<TSet<FIntPoint>>(
      CurrentPosition, DungeonLogicMap.Width, DungeonLogicMap.Height, foundUnit.movement * 2);
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

  if (CurrentGameState == GameState::SelectingAbility)
  {
    mapPawn->DisableInput(controller);
  }
  else
  {
    mapPawn->EnableInput(controller);
    if (queryTriggered)
    {
      if (CurrentGameState == GameState::Selecting)
      {
        if (!unitCanTakeAction)
          return;

        TWeakObjectPtr<ADungeonUnitActor> DungeonUnitActor = Game->unitIdToActor.FindChecked(*foundId);
        
        Dispatch(FChangeState(GameState::SelectingAbility));
      }
      else if (CurrentGameState == GameState::SelectingTarget)
      {
        auto id = lastSeenId != NULL ? *lastSeenId : -1;
        FMoveAction moveAction(id, (CurrentPosition));
        if (moveAction.canMove(this, lastMoveLocations))
        {
          Dispatch(MoveTemp(moveAction));
          Dispatch(FChangeState(GameState::Selecting));
        }
      }
    }
  }

  ReactMenu(CurrentGameState == GameState::SelectingAbility);
}

void ADungeonGameModeBase::ReactVisualization(TSet<FIntPoint> moveLocations, GameState GameState)
{
  if (GameState == SelectingTarget)
    return;
  this->lastMoveLocations = moveLocations;
  tileVisualizationComponent->Clear();
  tileVisualizationComponent->ShowTiles(moveLocations);
}

void ADungeonGameModeBase::ReactMenu(bool visible)
{
  MainCanvas->SetVisibility(visible ? EVisibility::Visible : EVisibility::Collapsed);
  RefocusMenu();
}

void ADungeonGameModeBase::RefocusMenu()
{
  if (MoveButton->HasUserFocus(0))
    return;
  FSlateApplication::Get().SetAllUserFocus(MoveButton);
}

void ADungeonGameModeBase::Tick(float deltaTime)
{
  Super::Tick(deltaTime);

  if (this->AnimationQueue.IsEmpty())
    return;

  (*this->AnimationQueue.Peek())->TickTimeline(deltaTime);
}

void ADungeonGameModeBase::BeginPlay()
{
  Super::BeginPlay();

  // auto thing = FString();
  // auto jsonObject = FJsonObjectConverter::UStructToJsonObject(FDungeonLogicMap{1, 1}).ToSharedRef();
  // auto writer = TJsonStringWriter<>::Create(&thing);
  // bool bSerialize = FJsonSerializer::Serialize(jsonObject, TJsonWriterFactory<>::Create(&thing));

  auto kisamaaa = FString(R"###(
        {
         "data":[86, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 86, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 86, 0, 0, 86, 0, 0, 0, 0, 0, 86, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 86, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 86, 0, 0, 0, 86, 0, 86, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
         "height":10,
         "id":1,
         "name":"Tile Layer 1",
         "opacity":1,
         "type":"tilelayer",
         "visible":true,
         "width":10,
         "x":0,
         "y":0
        }
  )###");
  TSharedPtr<FJsonObject> json;
  FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(kisamaaa), json);

  TArray<TSharedPtr<FJsonValue>> JsonValues = json->GetArrayField("data");

  int width = json->GetNumberField("width");
  int height = json->GetNumberField("height");

  for (int i = 0; i < height; ++i)
    for (int j = 0; j < width; ++j)
    {
      int dataIndex = i * height + j;
      auto value = JsonValues[dataIndex]->AsNumber();
      if (value > 0)
      {
        auto zaunit = FDungeonLogicUnit();
        zaunit.id = dataIndex;
        zaunit.name = TEXT("Test");
        zaunit.movement = 3;

        Game->map.loadedUnits.Add(zaunit.id, zaunit);

        FIntPoint positionPlacement{i, j};
        Game->map.unitAssignment.Add(positionPlacement, dataIndex);
      }
    }

  for (auto tuple : Game->map.unitAssignment)
  {
    auto unitActor = GetWorld()->SpawnActor<ADungeonUnitActor>(UnitActorPrefab, FVector::ZeroVector,
                                                               FRotator::ZeroRotator);
    Game->unitIdToActor.Add(tuple.Value, unitActor);
    unitActor->SetActorLocation(TilePositionToWorldPoint(tuple.Key));
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
    mapPawn->CursorEvent.AddUObject(this, &ADungeonGameModeBase::setCurrentPosition);
    mapPawn->QueryInput.AddUObject(this, &ADungeonGameModeBase::setCurrentPosition);

    auto newStyle = FCoreStyle::Get().GetWidgetStyle<FTextBlockStyle>("NormalText").Font;
    newStyle.Size = 32;

    SAssignNew(MainCanvas, SConstraintCanvas)
      + SConstraintCanvas::Slot()
        .Anchors(FAnchors(0.0f, 0.8f, 1.0f, 1.0f))
        .AutoSize(true)
      [
        SNew(SVerticalBox)
        + SVerticalBox::Slot()
        [SAssignNew(MoveButton, SButton)
        .IsFocusable(true)
        .HAlign(HAlign_Center)
        .VAlign(VAlign_Center)
          [SNew(STextBlock)
          .Font(newStyle)
          .Text(FText::FromString(TEXT("Move")))]]
        + SVerticalBox::Slot()
        [SAssignNew(WaitButton, SButton)
        .HAlign(HAlign_Center)
        .VAlign(VAlign_Center)
          [SNew(STextBlock)
          .Font(newStyle)
          .Text(FText::FromString(TEXT("Wait")))]]
      ];

    MoveButton->SetOnClicked(FOnClicked::CreateLambda(
        [this]() -> FReply
        {
          this->setCurrentGameState(GameState::SelectingTarget);
          return FReply::Handled();
        })
    );

    WaitButton->SetOnClicked(FOnClicked::CreateLambda(
      [this]() -> FReply
      {
        this->setCurrentGameState(GameState::Selecting);
        return FReply::Handled();
      }
    ));

    GetWorld()->GetGameViewport()->AddViewportWidgetContent(MainCanvas.ToSharedRef());
    this->setCurrentGameState(GameState::Selecting);
  }
}
