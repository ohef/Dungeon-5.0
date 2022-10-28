// Fill out your copyright notice in the Description page of Project Settings.


#include "Actor/TileVisualizationActor.h"

#define FastSubobject(FieldName) \
FieldName = CreateDefaultSubobject<TRemovePointer<decltype(FieldName)>::Type>(#FieldName);

ATileVisualizationActor::ATileVisualizationActor(const FObjectInitializer& ObjectInitializer)
{
  PrimaryActorTick.bCanEverTick = true;

  FastSubobject(MovementVisualizationComponent)
  FastSubobject(TileVisualizationComponent)
}

// Called when the game starts or when spawned
void ATileVisualizationActor::BeginPlay()
{
  Super::BeginPlay();
}

// Called every frame
void ATileVisualizationActor::Tick(float DeltaTime)
{
  Super::Tick(DeltaTime);
}
