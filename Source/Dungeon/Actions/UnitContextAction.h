#pragma once

#include "Action.h"
#include "UnitContextAction.generated.h"

USTRUCT(BlueprintType)
struct FUnitContextAction : public FAction
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