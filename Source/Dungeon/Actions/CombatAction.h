#pragma once

#include "UnitContextAction.h"
#include "Logic/unit.h"

#include "CombatAction.generated.h"

USTRUCT(BlueprintType)
struct FCombatAction : public FUnitContextAction
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite)
	int targetedUnit;
	UPROPERTY(BlueprintReadWrite)
	double damageValue;
};
