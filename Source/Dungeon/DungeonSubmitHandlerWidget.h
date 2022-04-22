// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "DungeonSubmitHandlerWidget.generated.h"

class USingleSubmitHandler;
/**
 * 
 */
UCLASS(Abstract)
class DUNGEON_API UDungeonSubmitHandlerWidget : public UUserWidget
{
  GENERATED_BODY()
  
public:
  UPROPERTY(meta=(BindWidget))
  UBorder* OuterCircle;
  UPROPERTY(meta=(BindWidget))
  UBorder* InnerCircle;
  UPROPERTY(meta=(BindWidget))
  UCanvasPanel* Container;
  UPROPERTY(meta=(BindWidgetAnim), Transient)
  UWidgetAnimation* OuterDissappear;
  
  FVector2D InitialOuterCircleSize;
  TWeakObjectPtr<USingleSubmitHandler> singleSubmitHandler;

  UFUNCTION()
  void HandleHit();
  
  virtual void NativeOnInitialized() override;
  virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
};
