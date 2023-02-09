// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include <Camera/CameraComponent.h>
#include <Math/IntPoint.h>

#include "Components/CapsuleComponent.h"
#include "Components/InterpToMovementComponent.h"
#include "Dungeon/Dungeon.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/SpringArmComponent.h"
#include "lager/reader.hpp"
#include "Logic/DungeonGameState.h"
#include "Utility/StoreConnectedClass.hpp"
#include "MapCursorPawn.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FCursorEvent, FIntPoint);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCursorEventDynamic, FIntPoint, point);

struct ZoomLevelNode
{
  int ZoomLevel;
  ZoomLevelNode* NextNode;
};

template <typename TElement>
struct TCycleArrayIterator
{
  TArray<TElement> iteratee;
  
  TCycleArrayIterator()
  {
  }

  bool CanCycle(){
    return !iteratee.IsEmpty();
  }

  TCycleArrayIterator(TArray<TElement> Iteratee)
    : iteratee(Iteratee)
  {
    previousIndex = iteratee.Num() - 1;
    currentIndex = 0;
  }

  decltype(auto) Forward()
  {
    previousIndex = currentIndex;
    currentIndex++;
    
    if (!iteratee.IsValidIndex(currentIndex))
      currentIndex = 0;
    
    return Current();
  }

  decltype(auto) Backwards()
  {
    previousIndex = currentIndex;
    currentIndex--;
    if (!iteratee.IsValidIndex(currentIndex))
      currentIndex = iteratee.Num() - 1;
    
    return Current();
  }

  int previousIndex;
  int currentIndex;

  TOptional<TElement> Current() 
  {
    if(iteratee.IsEmpty())
      return {};
    
    return { iteratee[currentIndex] };
  }
  
  TOptional<TElement> Previous() 
  {
    if(iteratee.IsEmpty())
      return {};
      
    return { iteratee[currentIndex] };
  }
};

UCLASS()
class AMapCursorPawn : public APawn, public FStoreConnectedClass<AMapCursorPawn, TDungeonAction>
{
  GENERATED_BODY()

public:
  AMapCursorPawn(const FObjectInitializer& ObjectInitializer);

protected:
  virtual void BeginPlay() override;

  void MoveRight(float Value);
  void MoveUp(float Value);
  void Cycle(TOptional<FIntPoint> (TCycleArrayIterator<FIntPoint>::*directionFunction)());
  void RotateCamera(float Value);
  void Query();
  void Next();
  void Previous();

  TArray<ZoomLevelNode> storedZoomLevels;
  float previousZoom;
  ZoomLevelNode* currentZoom;
  
  TSet<FIntPoint> interactionPoints;

  TCycleArrayIterator<FIntPoint> cycler;
  
public:
  void CycleZoom();
  
  lager::reader<FDungeonWorldState> reader;

  UFUNCTION(BlueprintCallable)
  virtual void Tick(float DeltaTime) override;

  virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

  FVector4 ConvertInputToCameraPlaneInput(FVector inputVector);

  FCursorEvent CursorEvent;
	UPROPERTY(BlueprintAssignable, Category="MapCursorPawn")
  FCursorEventDynamic DynamicCursorEvent;
  //Deprecated
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
  
  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  UInterpToMovementComponent* InterpToMovementComponent ;
  
  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  USpringArmComponent* SpringArmComponent;
};
