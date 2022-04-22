// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/FloatingCameraPawnMovement.h"

// Sets default values for this component's properties
UFloatingCameraPawnMovement::UFloatingCameraPawnMovement()
{
  // Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
  // off to improve performance if you don't need them.
  PrimaryComponentTick.bCanEverTick = true;

  // ...
}


// Called when the game starts
void UFloatingCameraPawnMovement::BeginPlay()
{
  Super::BeginPlay();

  // ...
  
}


// Called every frame
void UFloatingCameraPawnMovement::TickComponent(float DeltaTime, ELevelTick TickType,
                                                 FActorComponentTickFunction* ThisTickFunction)
{
  
  Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}
