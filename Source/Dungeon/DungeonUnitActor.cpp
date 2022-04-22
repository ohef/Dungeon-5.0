// Fill out your copyright notice in the Description page of Project Settings.


#include "DungeonUnitActor.h"


// Sets default values
ADungeonUnitActor::ADungeonUnitActor()
{
  // Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
  PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ADungeonUnitActor::BeginPlay()
{
  Super::BeginPlay();
  
}

// Called every frame
void ADungeonUnitActor::Tick(float DeltaTime)
{
  Super::Tick(DeltaTime);
}

