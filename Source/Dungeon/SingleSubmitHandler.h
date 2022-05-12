// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DungeonSubmitHandlerWidget.h"
#include "Actor/MapCursorPawn.h"
#include "Components/TimelineComponent.h"

#include "SingleSubmitHandler.generated.h"

class SSingleSubmitHandlerWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FIntervalHit);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class DUNGEON_API USingleSubmitHandler : public UActorComponent
{
  GENERATED_BODY()

public:
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IntervalHandler")
  float totalLength = 5.0;
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IntervalHandler")
  float pivot = 3.0;
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IntervalHandler")
  TArray<float> fallOffsFromPivot{0.3f, .5f, .75f};
  
  TFunction<void()> stopCheckingQueries;

  UFUNCTION(BlueprintCallable)
  float GetCurrentTimeInTimeline();

  UPROPERTY(BlueprintAssignable)
  FIntervalHit IntervalHit;

  UPROPERTY()
  UDungeonSubmitHandlerWidget* HandlerWidget;
  TSubclassOf<UDungeonSubmitHandlerWidget> HandlerWidgetClass;

  FTimeline timeline;
  FSlateBrush materialBrush;

  virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

  virtual void DoSubmit(FIntPoint);
private:
  TArray<FFloatInterval> handlers;

public:
  // Sets default values for this component's properties
  USingleSubmitHandler(const FObjectInitializer&);

protected:
  virtual void BeginPlay() override;

public:
  virtual void TickComponent(float DeltaTime, ELevelTick TickType,
                             FActorComponentTickFunction* ThisTickFunction) override;
};
