// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/TimelineComponent.h"
#include "Logic/DungeonGameState.h"
#include "Widget/DungeonSubmitHandlerWidget.h"

#include "SingleSubmitHandler.generated.h"

class SSingleSubmitHandlerWidget;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class DUNGEON_API USingleSubmitHandler : public UActorComponent/*, public FStoreConnectedClass<USingleSubmitHandler, TDungeonAction>*/
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
  UPROPERTY(EditAnywhere)
  FVector focusWorldLocation;
  UPROPERTY()
  UDungeonSubmitHandlerWidget* HandlerWidget;

  TDelegate<void(TOptional<FInteractionResults>)> interactionEnd;
  FInteractionFinished InteractionFinished;
  FInteractionResults results;

  TSubclassOf<UDungeonSubmitHandlerWidget> HandlerWidgetClass;

  FTimeline timeline;
  FSlateBrush materialBrush;

  virtual void DoSubmit();
  void TickOuterCircle(float DeltaTime);
  void Begin( TDelegate<void(TOptional<FInteractionResults>)> OnInteractionEnded, FVector worldLocationUnderFocus );
  
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
