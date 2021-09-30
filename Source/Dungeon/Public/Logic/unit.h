#pragma once

#include <CoreMinimal.h>
#include "Unit.generated.h"

UENUM()
enum UnitState {
  ActionTaken
};

USTRUCT()
struct FDungeonLogicUnit {
  GENERATED_BODY();

  UPROPERTY() int id;
  UPROPERTY() TEnumAsByte<UnitState> state;
  UPROPERTY() FString name;
  UPROPERTY() int movement;
};
