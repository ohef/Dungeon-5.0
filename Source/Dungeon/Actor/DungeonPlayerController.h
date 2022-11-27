// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Dungeon/Dungeon.h"
#include "GameFramework/PlayerController.h"
#include "Utility/StoreConnectedClass.hpp"
#include "DungeonGameModeBase.h"
#include "Rhythm/IntervalPriority.h"
#include "DungeonPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class DUNGEON_API ADungeonPlayerController : public APlayerController, public FStoreConnectedClass<ADungeonPlayerController, TDungeonAction>
{
  GENERATED_BODY()
public:
  ADungeonPlayerController(const FObjectInitializer& ObjectInitializer);
  
  FQueryInput QueryInput;

  virtual void Tick(float DeltaSeconds) override;
  void HandleQuery();
  void GoBackInteraction();
  void HandleEnter();
  virtual void BeginPlay() override;
	
  void BeginInteraction(TOptional<FInteractionResults> interactionResults);

  lager::reader<TOptional<FUnitInteraction>> interactionReader;

  virtual void SetupInputComponent() override;
};
