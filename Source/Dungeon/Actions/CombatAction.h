#pragma once

#include "UnitContextAction.h"
#include "Logic/unit.h"

#include "CombatAction.generated.h"

USTRUCT(BlueprintType)
struct FCombatAction : public FUnitContextAction
{
  GENERATED_BODY()

  FCombatAction() = default;

  FCombatAction(int InitiatorId)
    : FUnitContextAction(InitiatorId)
  {
  }

  FCombatAction(int InitiatorId, const FDungeonLogicUnit& UpdatedUnit, double DamageValue, const FIntPoint& Target)
    : FUnitContextAction(InitiatorId),
      updatedUnit(UpdatedUnit),
      damageValue(DamageValue),
      target(Target)
  {
  }

  UPROPERTY(BlueprintReadWrite)
  FDungeonLogicUnit updatedUnit;
  UPROPERTY(BlueprintReadWrite)
  double damageValue;
  UPROPERTY(BlueprintReadWrite)
  FIntPoint target;
};
