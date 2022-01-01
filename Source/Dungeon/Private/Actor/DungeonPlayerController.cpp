// Fill out your copyright notice in the Description page of Project Settings.


#include "Actor/DungeonPlayerController.h"

#include "DungeonConstants.h"
#include "Dungeon/DungeonGameModeBase.h"

void ADungeonPlayerController::SetupInputComponent()
{
  Super::SetupInputComponent();
	
	auto controller = static_cast<ADungeonPlayerController*>(GetWorld()->GetFirstPlayerController());
  ADungeonGameModeBase* gamemode = static_cast<ADungeonGameModeBase*>(GetWorld()->
	  GetAuthGameMode());
  auto& binding = controller->InputComponent->BindAction(GKeyFocus, EInputEvent::IE_Pressed,
                                                         gamemode, &ADungeonGameModeBase::RefocusThatShit);
	binding.bConsumeInput = false;
}
