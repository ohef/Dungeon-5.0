// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "lager/reader.hpp"
#include "Logic/DungeonGameState.h"
#include "Logic/unit.h"
#include "Utility/StoreConnectedClass.hpp"
#include "DungeonGameModeBase.h"
#include "Widget/DamageWidget.h"
#include "DungeonUnitActor.generated.h"

struct FDungeonUnitActorHandler;

UCLASS()
class DUNGEON_API ADungeonUnitActor : public AActor, public FStoreConnectedClass<ADungeonUnitActor, TDungeonAction>
{
  GENERATED_BODY()

  friend FDungeonUnitActorHandler;
  
public:
  // Sets default values for this actor's properties
  ADungeonUnitActor();

protected:
  // Called when the game starts or when spawned
  virtual void BeginPlay() override;

public:
  
  int id;
  FIntPoint lastPosition;
  
  using FReaderType = std::tuple<FDungeonLogicUnit, FIntPoint, TOptional<int>>;
  lager::reader<FReaderType> reader;
  lager::reader<TTuple<FIntPoint, FIntPoint>> wew;
  
  void HookIntoStore();
  void HandleGlobalEvent(const TDungeonAction& action);
  
  UFUNCTION(BlueprintImplementableEvent, Category="DungeonUnit")
  void React(FDungeonLogicUnit updatedState);
  
  UFUNCTION(BlueprintImplementableEvent, Category="DungeonUnit")
  void ReactCombatAction(FCombatAction updatedState);
  
  UPROPERTY(EditAnywhere,BlueprintReadWrite)
  UInterpToMovementComponent* InterpToMovementComponent;
  UPROPERTY(EditAnywhere,BlueprintReadWrite)
  USceneComponent* PathRotation;
  UPROPERTY(Transient)
  TSubclassOf<UDamageWidget> DamageWidgetClass;
  UPROPERTY(Transient)
  UDamageWidget* DamageWidget;
  
  virtual void Tick(float DeltaTime) override;
};
