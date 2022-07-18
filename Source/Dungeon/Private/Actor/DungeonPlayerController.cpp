// Fill out your copyright notice in the Description page of Project Settings.


#include "Actor/DungeonPlayerController.h"

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
  Cast<ADungeonGameModeBase>(GetWorld()->GetAuthGameMode())->GoBackOnInputState();
}

void ADungeonPlayerController::SetupInputComponent()
{
  Super::SetupInputComponent();

  auto controller = static_cast<ADungeonPlayerController*>(GetWorld()->GetFirstPlayerController());
  auto& binding = controller->InputComponent->BindAction(GQuery, EInputEvent::IE_Pressed, this, &ADungeonPlayerController::RaiseQuery);
  binding.bConsumeInput = false;
  
  controller->InputComponent->BindAction(GGoBack, EInputEvent::IE_Pressed, this, &ADungeonPlayerController::GoBackInteraction)
  .bConsumeInput = false;

  // auto controller = static_cast<ADungeonPlayerController*>(GetWorld()->GetFirstPlayerController());
  //  ADungeonGameModeBase* gamemode = static_cast<ADungeonGameModeBase*>(GetWorld()->
  //   GetAuthGameMode());
  //  auto& binding = controller->InputComponent->BindAction(GKeyFocus, EInputEvent::IE_Pressed, gamemode, &ADungeonGameModeBase::RefocusThatShit);
  // binding.bConsumeInput = false;
}
