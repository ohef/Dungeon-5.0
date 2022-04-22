// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Logic/unit.h"
#include "DungeonUnitActor.generated.h"

UCLASS()
class DUNGEON_API ADungeonUnitActor : public AActor
{
  GENERATED_BODY()

public:
  // Sets default values for this actor's properties
  ADungeonUnitActor();

protected:
  // Called when the game starts or when spawned
  virtual void BeginPlay() override;

public:
  UFUNCTION(BlueprintImplementableEvent, Category="DungeonUnit")
  void React(FDungeonLogicUnit updatedState);
  // Called every frame
  virtual void Tick(float DeltaTime) override;
};
