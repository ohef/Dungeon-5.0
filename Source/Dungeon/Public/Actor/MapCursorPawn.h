// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include <Camera/CameraComponent.h>
#include <Math/IntPoint.h>
#include <Math/Vector2D.h>
#include <functional>

#include <Dungeon/ThirdParty/flowcpp/flow.h>
#include <Dungeon/Public/Logic/map.h>
#include <Dungeon/DungeonGameModeBase.h>

#include "MapCursorPawn.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FCursorEvent, FIntPoint);

DECLARE_DELEGATE_OneParam(FQueryPoint, FIntPoint);

UCLASS()
class AMapCursorPawn : public APawn
  //, public DispatchMixin<AMapCursorPawn>
{
  GENERATED_BODY()

public:
  // Sets default values for this pawn's properties
  AMapCursorPawn();

protected:
  // Called when the game starts or when spawned
  virtual void BeginPlay() override;
  
  void MoveRight(float Value);
  void MoveUp(float Value);
  void RotateCamera(float Value);
  void Query();

  FIntPoint CurrentPosition = FIntPoint(0, 0);

public:
  FQueryPoint QueryPoint;

  virtual void Tick(float DeltaTime) override;

  virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

  FCursorEvent CursorEvent;

  UPROPERTY(VisibleAnywhere, BlueprintReadWrite) UCameraComponent* Camera;

  UPROPERTY(VisibleAnywhere) UStaticMeshComponent* CursorMesh;

  UPROPERTY(VisibleAnywhere) float discreteWorldScale;
};
