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

void ADungeonPlayerController::HandleQuery()
{
  // QueryInput.Broadcast(FIntPoint());
  GetWorld()
  ->GetAuthGameMode<ADungeonGameModeBase>()
  ->SingleSubmitHandler
  ->DoSubmit();
}

void ADungeonPlayerController::GoBackInteraction()
{
  GetWorld()
  ->GetAuthGameMode<ADungeonGameModeBase>()
  ->Dispatch(TDungeonAction(TInPlaceType<FBackAction>{}));
}

void ADungeonPlayerController::HandleEnter(){
}

void ADungeonPlayerController::BeginPlay()
{
  Super::BeginPlay();
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
