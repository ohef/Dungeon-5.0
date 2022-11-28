// Fill out your copyright notice in the Description page of Project Settings.


#include "DungeonUnitActor.h"

#include "DungeonConstants.h"
#include "DungeonGameModeBase.h"
#include "Kismet/KismetSystemLibrary.h"
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
  reader = this->GetWorld()
               ->template GetAuthGameMode<ADungeonGameModeBase>()
               ->store
               ->zoom(lager::lenses::fan(thisUnitLens(id), unitIdToPosition(id), isUnitFinishedLens2(id)))
               .make();
  
  UseEvent().AddUObject(this, &ADungeonUnitActor::HandleGlobalEvent);

  reader.bind([&](const auto& ReaderVal)
  {
    FDungeonLogicUnit DungeonLogicUnit = lager::view(first, ReaderVal);
    const FIntPoint& IntPoint = lager::view(second, ReaderVal);
    DungeonLogicUnit.state = lager::view(element<2>, ReaderVal).IsSet() ? UnitState::ActionTaken : UnitState::Free;

    UKismetSystemLibrary::PrintString(this, DungeonLogicUnit.Name);
    UKismetSystemLibrary::PrintString(this, IntPoint.ToString());

    this->React(DungeonLogicUnit);
    this->SetActorLocation(TilePositionToWorldPoint(IntPoint));
  });
}

void ADungeonUnitActor::HandleGlobalEvent(const TDungeonAction& action) {
  Visit([this](auto&& event)
  {
    using TEventType = typename TDecay<decltype(event)>::Type;
    if constexpr (TIsSame<FCombatAction, TEventType>::Value)
    {
      if(lager::view(first, reader.get()).Id == event.InitiatorId)
      {
        UKismetSystemLibrary::PrintString(this, "your life means everything, you serve ALL purpose");
      }
    }
  }, action);
}

// Called every frame
void ADungeonUnitActor::Tick(float DeltaTime)
{
  Super::Tick(DeltaTime);
}
