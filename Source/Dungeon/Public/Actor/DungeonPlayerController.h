// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Dungeon/Dungeon.h"
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
  ADungeonPlayerController(const FObjectInitializer& ObjectInitializer);
  
  FQueryInput QueryInput;

  virtual void Tick(float DeltaSeconds) override;
  void RaiseQuery();
  void GoBackInteraction();

  virtual void SetupInputComponent() override;
};
