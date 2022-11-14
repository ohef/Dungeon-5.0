// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/TileVisualizationComponent.h"
#include "GameFramework/Actor.h"
#include "TileVisualizationActor.generated.h"

UCLASS()
class DUNGEON_API ATileVisualizationActor : public AActor, public FStoreConnectedClass<ATileVisualizationActor, TAction>
{
  GENERATED_BODY()

public:
  // Sets default values for this actor's properties
  ATileVisualizationActor(const FObjectInitializer& ObjectInitializer);

  UPROPERTY(EditAnywhere,BlueprintReadWrite)
  UTileVisualizationComponent* TileVisualizationComponent;
  UPROPERTY(EditAnywhere,BlueprintReadWrite)
  UTileVisualizationComponent* MovementVisualizationComponent;

  lager::reader<TInteractionContext> contextCursor;

protected:
  // Called when the game starts or when spawned
  virtual void BeginPlay() override;

public:
  // Called every frame
  virtual void Tick(float DeltaTime) override;
};
