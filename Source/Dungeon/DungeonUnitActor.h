// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "lager/reader.hpp"
#include "Logic/DungeonGameState.h"
#include "Logic/unit.h"
#include "Utility/StoreConnectedClass.hpp"
#include "DungeonGameModeBase.h"
#include "DungeonUnitActor.generated.h"

UCLASS()
class DUNGEON_API ADungeonUnitActor : public AActor, public FStoreConnectedClass<ADungeonUnitActor, TDungeonAction>
{
  GENERATED_BODY()
  
public:
  // Sets default values for this actor's properties
  ADungeonUnitActor();

protected:
  // Called when the game starts or when spawned
  virtual void BeginPlay() override;

public:
  
  int id;
  // lager::reader<FDungeonLogicUnit> reader;
  using FReaderType = std::tuple<FDungeonLogicUnit, FIntPoint, TOptional<int>>;
  lager::reader<FReaderType> reader;
  void hookIntoStore();
  void HandleGlobalEvent(const TDungeonAction& action);
  
  UFUNCTION(BlueprintImplementableEvent, Category="DungeonUnit")
  void React(FDungeonLogicUnit updatedState);
  
  virtual void Tick(float DeltaTime) override;
};
