// Fill out your copyright notice in the Description page of Project Settings.


#include "Actor/DungeonPlayerController.h"

#include "DungeonConstants.h"
#include "Dungeon/DungeonGameModeBase.h"

ADungeonPlayerController::ADungeonPlayerController(const FObjectInitializer& ObjectInitializer): Super(
  ObjectInitializer)
{
  bBlockInput = false;
}

void ADungeonPlayerController::Tick(float DeltaSeconds)
{
  Super::Tick(DeltaSeconds);
}

void ADungeonPlayerController::RaiseQuery()
{
  QueryInput.Broadcast(FIntPoint());
}

void ADungeonPlayerController::GoBackInteraction()
{
  GetWorld()
  ->GetAuthGameMode<ADungeonGameModeBase>()
  ->Dispatch(TAction(TInPlaceType<FBackAction>{}));
}

void ADungeonPlayerController::SetupInputComponent()
{
  Super::SetupInputComponent();

  auto controller = static_cast<ADungeonPlayerController*>(GetWorld()->GetFirstPlayerController());
  controller->InputComponent->BindAction(GQuery, EInputEvent::IE_Pressed, this, &ADungeonPlayerController::RaiseQuery)
  .bConsumeInput = false;
  
  controller->InputComponent->BindAction(GOpenMenu, EInputEvent::IE_Pressed, this, &ADungeonPlayerController::GoBackInteraction)
  .bConsumeInput = false;
  
  controller->InputComponent->BindAction(GGoBack, EInputEvent::IE_Pressed, this, &ADungeonPlayerController::GoBackInteraction)
  .bConsumeInput = false;
}