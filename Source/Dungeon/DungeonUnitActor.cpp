// Fill out your copyright notice in the Description page of Project Settings.


#include "DungeonUnitActor.h"

#include "lager/lenses/tuple.hpp"
#include "Lenses/model.hpp"

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

void ADungeonUnitActor::hookIntoStore()
{
  reader = lager::view(worldStoreLens, GetWorld())
           ->zoom(lager::lenses::fan(thisUnitLens(id), unitAtPoint(id)))
           .make();

  reader.bind([&](readerVal_t ReaderVal)
  {
    const FDungeonLogicUnit& DungeonLogicUnit = lager::view(first, ReaderVal);
    const FIntPoint& IntPoint = lager::view(second, ReaderVal);

    UKismetSystemLibrary::PrintString(this, DungeonLogicUnit.Name);
    UKismetSystemLibrary::PrintString(this, IntPoint.ToString());

    this->React(DungeonLogicUnit);
    this->SetActorLocation(TilePositionToWorldPoint(IntPoint));
  });
}

// Called every frame
void ADungeonUnitActor::Tick(float DeltaTime)
{
  Super::Tick(DeltaTime);
}
