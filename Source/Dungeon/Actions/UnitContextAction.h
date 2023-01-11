#pragma once

#include "UnitContextAction.generated.h"

USTRUCT(BlueprintType)
struct FUnitContextAction 
{
  GENERATED_BODY()

  UPROPERTY(BlueprintReadWrite)
  int InitiatorId;

  FUnitContextAction() = default;

  FUnitContextAction(int InitiatorId)
    : InitiatorId(InitiatorId)
  {
  }
};

using FWaitAction = FUnitContextAction;