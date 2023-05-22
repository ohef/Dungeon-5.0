// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DungeonGameModeBase.h"
#include "Blueprint/UserWidget.h"
#include "Lenses/model.hpp"
#include "Logic/DungeonGameState.h"
#include "Utility/StoreConnectedClass.hpp"
#include "TurnNotifierWidget.generated.h"

/**
 * 
 */
UCLASS()
class DUNGEON_API UTurnNotifierWidget : public UUserWidget, public FStoreConnectedClass<UTurnNotifierWidget, TDungeonAction>
{
  GENERATED_BODY()

public: 
  UPROPERTY(meta=(BindWidgetAnim), Transient)
  UWidgetAnimation* FadeIn;

  lager::reader<FText> CurrentTurn;

  UFUNCTION(BlueprintPure)
  const FText& GetCurrentTurn() { return CurrentTurn.get(); }

  virtual bool Initialize() override;
  void PlayMyAnimation();
};
