// Copyright Epic Games, Inc. All Rights Reserved.


#include "DungeonGameModeBase.h"

#include <Logic/game.h>
#include <Data/actions.h>

#include "DungeonUserWidget.h"
#include "LocalizationTargetTypes.h"
#include "Actor/DungeonPlayerController.h"
#include "Actor/MapCursorPawn.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "ThirdParty/immer/vector.hpp"

class FActionDispatcher : public ActionVisitor
{
  ADungeonGameModeBase* gameModeBase;

public:
  FActionDispatcher(ADungeonGameModeBase* gameModeBase) : gameModeBase(gameModeBase)
  {
  }

  FDungeonLogicMap Visit(const MovementAction& ability) const override
  {
    auto& state = gameModeBase->Game->map;
    auto possibleLocation = state.unitAssignment.FindKey(ability.unitID);
    if (possibleLocation == nullptr) return state;
    auto key = *possibleLocation;
    state.unitAssignment.Remove(key);
    state.unitAssignment.Add(ability.to, ability.unitID);
    auto unit = gameModeBase->Game->unitIdToActor.Find(ability.unitID);
    if (unit == nullptr)
      return state;
    auto zaValue = gameModeBase->Game->map.unitAssignment.FindKey(ability.unitID);
    (*unit)->SetActorLocation(TilePositionToWorldPoint(*zaValue));
    return state;
  }

  FDungeonLogicMap Visit(const WaitAction& ability) const override
  {
    auto& state = gameModeBase->Game->map;
    return state;
  }

  FDungeonLogicMap Visit(const CommitAction& ability) const override
  {
    auto& state = gameModeBase->Game->map;
    return state;
  }
};

TileLayer ADungeonGameModeBase::colorVisualizer(TSet<FIntPoint> targetsSet, FLinearColor color, AActor* manager)
{
  auto component = static_cast<UTileVisualizationComponent*>(manager->AddComponentByClass(
    UTileVisualizationComponent::StaticClass(), false, FTransform::Identity, false));
  component->ShowTiles(targetsSet);
  return component;
}

ADungeonGameModeBase::ADungeonGameModeBase(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
  PrimaryActorTick.bCanEverTick = true;
  Game = CreateDefaultSubobject<UDungeonLogicGame>("own");
  Game->Init();
}

void ADungeonGameModeBase::MoveUnit(FIntPoint point, int unitID)
{
  MovementAction mab;
  mab.unitID = unitID;
  mab.to = point;
  FActionDispatcher arv = FActionDispatcher(this);
  arv.Visit(mab);
}

void ADungeonGameModeBase::RefocusThatShit()
{
  if (MenuRef == nullptr || !MenuRef->Parent.IsValid())
    return;

  TObjectPtr<UWidget> focusThis = MenuRef->Parent->GetSlots()[0]->Content;
  APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
  if (focusThis->HasUserFocus(PlayerController))
    return;
  focusThis->SetUserFocus(PlayerController);
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

  for (auto tuple : Game->map.unitAssignment)
  {
    AActor* unitActor = GetWorld()->SpawnActor<AActor>(UnitActorPrefab, FVector::ZeroVector, FRotator::ZeroRotator);
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
  tileShowPrefab->SetActorLocation({50, 50, 0});
  UTileVisualizationComponent* component = static_cast<UTileVisualizationComponent*>(tileShowPrefab->
    GetComponentByClass(UTileVisualizationComponent::StaticClass()));

  AMapCursorPawn* mapPawn = static_cast<AMapCursorPawn*>(GetWorld()->GetFirstPlayerController()->GetPawn());
  if (mapPawn != NULL)
  {
    mapPawn->CursorEvent.AddLambda([&, component, tileShowPrefab](FIntPoint pt)
    {
      if (isSelecting) return;

      UKismetSystemLibrary::PrintString(this, pt.ToString(), true, false);
      FDungeonLogicMap& DungeonLogicMap = Game->map;

      TMap<FIntPoint, int>& Tuples = DungeonLogicMap.unitAssignment;
      if (Tuples.Contains(pt))
      {
        auto& foundUnit = DungeonLogicMap.loadedUnits.FindChecked(Tuples.FindChecked(pt));
        TArray<FIntPoint> moveLocations = manhattanReachablePoints(pt, DungeonLogicMap.Width, DungeonLogicMap.Height,
                                                                   foundUnit.movement * 2);
        component->ShowTiles(TSet<FIntPoint>(moveLocations));
      }
      else
      {
        if (component->GetInstanceCount() > 0)
          component->Clear();
      }
    });

    actionQueryHandler = [this, mapPawn, component](FIntPoint point)
    {
      MoveUnit(point, 1);
      isSelecting = false;
      component->Clear();
      mapPawn->QueryPoint.BindWeakLambda(this, baseQueryHandler);
    };

    MenuRef = NewObject<UDungeonUserWidget>(this, MenuClass);
    MenuRef->Initialize();
    MenuRef->AddToViewport();
    MenuRef->SetVisibility(ESlateVisibility::Collapsed);

    baseQueryHandler = [mapPawn, this](FIntPoint pt)
    {
      //Run some checks to see if we can do something
      if (Game->map.unitAssignment.Contains(pt))
      {
        //Now transition the state
        mapPawn->DisableInput(GetWorld()->GetFirstPlayerController());
        MenuRef->Update({true});
        RefocusThatShit();
        TArray<UPanelSlot*> menuSlots = MenuRef->Parent->GetSlots();
        TObjectPtr<UButton> moveButton = Cast<UButton>(menuSlots[0]->Content);
        TObjectPtr<UButton> waitButton = Cast<UButton>(menuSlots[1]->Content);
        if (moveButton)
        {
          auto button = StaticCastSharedPtr<SButton>(moveButton->GetCachedWidget());
          FOnClicked MenuHandler;
          MenuHandler.BindLambda([this, mapPawn]() -> FReply
          {
            if (this != nullptr)
            {
              UKismetSystemLibrary::PrintString(this, "Moving");
              MenuRef->Update({false});
              mapPawn->EnableInput(this->GetWorld()->GetFirstPlayerController());
              this->isSelecting = true;
              mapPawn->QueryPoint.BindWeakLambda(this, actionQueryHandler);
            }
            return FReply::Handled();
          });
          button->SetOnClicked(MenuHandler);
        }

        if (waitButton)
        {
          auto button = StaticCastSharedPtr<SButton>(waitButton->GetCachedWidget());
          FOnClicked MenuHandler;
          MenuHandler.BindLambda([mapPawn](TWeakObjectPtr<ADungeonGameModeBase> theWeak) -> FReply
          {
            if (theWeak.IsValid())
            {
              UKismetSystemLibrary::PrintString(theWeak.Get(), "Wait");
              theWeak->MenuRef->Update({false});
              mapPawn->EnableInput(theWeak->GetWorld()->GetFirstPlayerController());
            }
            return FReply::Handled();
          }, MakeWeakObjectPtr(this));
          button->SetOnClicked(MenuHandler);
        }
      }
    };

    mapPawn->QueryPoint.BindWeakLambda(this, baseQueryHandler);
  }
}
