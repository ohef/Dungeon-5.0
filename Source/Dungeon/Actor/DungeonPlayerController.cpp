// Fill out your copyright notice in the Description page of Project Settings.


#include "Actor/DungeonPlayerController.h"

#include "DungeonConstants.h"
#include "Dungeon/DungeonGameModeBase.h"
#include "Lenses/model.hpp"

ADungeonPlayerController::ADungeonPlayerController(const FObjectInitializer& ObjectInitializer): Super(
  ObjectInitializer)
{
  bBlockInput = false;
}

void ADungeonPlayerController::BeginPlay()
{
  interactionReader = UseState(interactionContextLens | unreal_alternative<FUnitInteraction> ).make();
  interactionReader.watch([this](const TOptional<FUnitInteraction>& UnitInteraction)
  {
    if(UnitInteraction.IsSet()){
      FIntPoint Point = UseViewState(unitIdToPosition(UnitInteraction->targetIDUnderFocus));
      GetWorld()
        ->GetAuthGameMode<ADungeonGameModeBase>()
        ->SingleSubmitHandler
        ->Begin(TDelegate<void(TOptional<FInteractionResults>)>::CreateUObject(
                  this, &ADungeonPlayerController::BeginInteraction),
                TilePositionToWorldPoint(Point));
    }
    else
    {
      GetWorld()
        ->GetAuthGameMode<ADungeonGameModeBase>()
        ->SingleSubmitHandler
        ->HandlerWidget
        ->Stop();
    }
  });
  
  Super::BeginPlay();
}

void ADungeonPlayerController::BeginInteraction(TOptional<FInteractionResults> interactionResults)
{
  if(interactionResults.IsSet())
    StoreDispatch(TDungeonAction(TInPlaceType<FTimingInteractionResults>{}, interactionResults.GetValue()));
}

void ADungeonPlayerController::Tick(float DeltaSeconds)
{
  Super::Tick(DeltaSeconds);
}

void ADungeonPlayerController::HandleQuery()
{
  if (UseViewState(interactionContextLens | unreal_alternative<FUnitInteraction>).IsSet())
  {
    GetWorld()
      ->GetAuthGameMode<ADungeonGameModeBase>()
      ->SingleSubmitHandler
      ->DoSubmit();
  }
}

void ADungeonPlayerController::GoBackInteraction()
{
  GetWorld()
  ->GetAuthGameMode<ADungeonGameModeBase>()
  ->Dispatch(TDungeonAction(TInPlaceType<FBackAction>{}));
}

void ADungeonPlayerController::SetupInputComponent()
{
  Super::SetupInputComponent();

  auto controller = static_cast<ADungeonPlayerController*>(GetWorld()->GetFirstPlayerController());
  controller->InputComponent->BindAction(GQuery, EInputEvent::IE_Pressed, this, &ADungeonPlayerController::HandleQuery)
  .bConsumeInput = false;
  
  controller->InputComponent->BindAction(GOpenMenu, EInputEvent::IE_Pressed, this, &ADungeonPlayerController::GoBackInteraction)
  .bConsumeInput = false;
  
  controller->InputComponent->BindAction(GGoBack, EInputEvent::IE_Pressed, this, &ADungeonPlayerController::GoBackInteraction)
  .bConsumeInput = false;
}
