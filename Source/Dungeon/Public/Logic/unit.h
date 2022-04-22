#pragma once

#include <CoreMinimal.h>
#include "Unit.generated.h"

UENUM(BlueprintType)
enum UnitState
{
  Free,
  ActionTaken
};

USTRUCT(BlueprintType)
struct FDungeonLogicUnit
{
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly, Category=DungeonUnit)
  int id;
  UPROPERTY(BlueprintReadOnly, Category=DungeonUnit)
  int damage;
  UPROPERTY(BlueprintReadOnly, Category=DungeonUnit)
  int hitPoints;
  UPROPERTY(BlueprintReadOnly, Category=DungeonUnit)
  TEnumAsByte<UnitState> state;
  UPROPERTY(BlueprintReadOnly, Category=DungeonUnit)
  FString name;
  UPROPERTY(BlueprintReadOnly, Category=DungeonUnit)
  int movement;
};
