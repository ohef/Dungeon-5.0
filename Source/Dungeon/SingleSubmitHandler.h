// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DungeonSubmitHandlerWidget.h"
#include "Components/TimelineComponent.h"

#include "SingleSubmitHandler.generated.h"

class SSingleSubmitHandlerWidget;

struct FIntervalPriority : public FFloatInterval
{
  int8 order;
  float hitTime;

  using FFloatInterval::FFloatInterval;

  FIntervalPriority(float InMin, float InMax, int8 order)
    : FFloatInterval(InMin, InMax), order(order)
  {
  }
};

typedef TArray<FIntervalPriority> FInteractionResults;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class DUNGEON_API USingleSubmitHandler : public UActorComponent
{
  GENERATED_BODY()

  DECLARE_EVENT_OneParam(USingleSubmitHandler, FInteractionFinished, const FInteractionResults&);

public:
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IntervalHandler")
  float totalLength = 5.0;
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IntervalHandler")
  float pivot = 3.0;
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IntervalHandler")
  TArray<float> fallOffsFromPivot{0.3f, .5f, .75f};

  TFunction<void()> stopCheckingQueries;

  UPROPERTY(EditAnywhere)
  FVector focusWorldLocation;

  UFUNCTION(BlueprintCallable)
  float GetCurrentTimeInTimeline();
  void EndInteraction();

  FInteractionFinished InteractionFinished;
  FInteractionResults results;

  UPROPERTY()
  UDungeonSubmitHandlerWidget* HandlerWidget;
  TSubclassOf<UDungeonSubmitHandlerWidget> HandlerWidgetClass;

  FTimeline timeline;
  FSlateBrush materialBrush;

  virtual void DoSubmit(FIntPoint);
  TArray<FIntervalPriority> handlers;
  
public:
  // Sets default values for this component's properties
  USingleSubmitHandler(const FObjectInitializer&);

  UFUNCTION()
  void RemoveAfterAnimationFinished();

protected:
  virtual void BeginPlay() override;

public:
  virtual void TickComponent(float DeltaTime, ELevelTick TickType,
                             FActorComponentTickFunction* ThisTickFunction) override;
};
