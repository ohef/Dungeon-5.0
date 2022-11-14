// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TurnNotifierWidget.generated.h"

/**
 * 
 */
UCLASS()
class DUNGEON_API UTurnNotifierWidget : public UUserWidget
{
  GENERATED_BODY()

public: 
  UPROPERTY(meta=(BindWidgetAnim), Transient)
  UWidgetAnimation* FadeIn;
  virtual bool Initialize() override;
  void PlayMyAnimation();
};
