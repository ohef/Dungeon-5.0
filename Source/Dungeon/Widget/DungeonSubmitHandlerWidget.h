// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Rhythm/IntervalPriority.h"

#include "DungeonSubmitHandlerWidget.generated.h"

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
  UCanvasPanel* Container;
  UPROPERTY(meta=(BindWidgetAnim), Transient)
  UWidgetAnimation* OuterDissappear;
  UPROPERTY(BlueprintReadWrite)
  UMaterialInterface* CircleMaterial;
  UPROPERTY(Transient)
  UMaterialInstanceDynamic* InnerCircleMaterial;

  UDungeonSubmitHandlerWidget(const FObjectInitializer& Initializer);
  
  FVector2D InitialOuterCircleSize;
  TArray<TWeakObjectPtr<UWidget>> addedCircles;

  //Stuff
  void RenderProperties(TArray<FIntervalPriority> IntervalPrioritiess, float TimelineLengthh, float PlaybackPositionn);
  TArray<FIntervalPriority> IntervalPriorities;
  float TimelineLength;
  float PlaybackPosition;

  UFUNCTION()
  void Stop();
  UFUNCTION()
  void HandleHit();
  
  virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
  virtual void NativeOnInitialized() override;
};
