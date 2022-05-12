// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include <Camera/CameraComponent.h>
#include <Math/IntPoint.h>

#include <Dungeon/ThirdParty/flowcpp/flow.h>
#include <Dungeon/DungeonGameModeBase.h>

#include "Components/CapsuleComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "MapCursorPawn.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FCursorEvent, FIntPoint);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCursorEventDynamic, FIntPoint, point);
DECLARE_EVENT_OneParam(AMapCursorPawn, FQueryInput, FIntPoint);

UCLASS()
class AMapCursorPawn : public APawn
{
  GENERATED_BODY()

public:
  AMapCursorPawn(const FObjectInitializer& ObjectInitializer);

protected:
  virtual void BeginPlay() override;

  void MoveRight(float Value);
  void MoveUp(float Value);
  void RotateCamera(float Value);
  void Query();
  
  UPROPERTY()
  bool QueryCalled = false;
  
public:
  bool ConsumeQueryCalled()
  {
    auto current = QueryCalled;
    QueryCalled = false;;
    return current;
  };

  UFUNCTION(BlueprintCallable)
  virtual void Tick(float DeltaTime) override;

  virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

  FVector4 ConvertInputToCameraPlaneInput(FVector inputVector);

  FCursorEvent CursorEvent;
	UPROPERTY(BlueprintAssignable, Category="MapCursorPawn")
  FCursorEventDynamic DynamicCursorEvent;
  FQueryInput QueryInput;

  UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
  UDrawFrustumComponent* FrustumComponent;

  UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
  FIntPoint CurrentPosition = FIntPoint(0, 0);

  UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
  UCameraComponent* Camera;

  UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
  USceneComponent* Offset;
  
  UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
  USceneComponent* CursorMesh;

  UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
  UCapsuleComponent* CursorCollider;

  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  UFloatingPawnMovement* MovementComponent ;
};
