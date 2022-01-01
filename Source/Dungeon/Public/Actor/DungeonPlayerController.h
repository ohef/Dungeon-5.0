// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "DungeonPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class DUNGEON_API ADungeonPlayerController : public APlayerController
{
  GENERATED_BODY()
public:
  explicit ADungeonPlayerController(const FObjectInitializer& ObjectInitializer)
    : APlayerController(ObjectInitializer)
  {
    bBlockInput = false;
  }

  virtual void SetupInputComponent() override;
};
