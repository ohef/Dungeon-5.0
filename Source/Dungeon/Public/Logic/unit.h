#pragma once

#include <CoreMinimal.h>

#include "Engine/DataTable.h"
#include "Unit.generated.h"

class ADungeonUnitActor;

UENUM(BlueprintType)
enum UnitState
{
  Free,
  ActionTaken
};

USTRUCT(BlueprintType)
struct FDungeonLogicUnit : public FTableRowBase
{
  GENERATED_BODY()

  FDungeonLogicUnit(): id(0), damage(0), hitPoints(0), state(), movement(0), teamId(0), attackRange(0)
  {
  }

  FDungeonLogicUnit(const FDungeonLogicUnit& Other)
    : id(Other.id),
      damage(Other.damage),
      hitPoints(Other.hitPoints),
      state(Other.state),
      name(Other.name),
      movement(Other.movement),
      teamId(Other.teamId),
      attackRange(Other.attackRange)
  {
  }

  FDungeonLogicUnit& operator=(const FDungeonLogicUnit& Other)
  {
    if (this == &Other)
      return *this;
    id = Other.id;
    damage = Other.damage;
    hitPoints = Other.hitPoints;
    state = Other.state;
    name = Other.name;
    movement = Other.movement;
    teamId = Other.teamId;
    attackRange = Other.attackRange;
    return *this;
  }

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=DungeonUnit)
  int id;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=DungeonUnit)
  int damage;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=DungeonUnit)
  int hitPoints;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=DungeonUnit)
  TEnumAsByte<UnitState> state;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=DungeonUnit)
  FString name;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=DungeonUnit)
  int movement;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=DungeonUnit)
  int teamId;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=DungeonUnit)
  int attackRange;
  int getAttackRange();
};

USTRUCT()
struct FDungeonLogicUnitRow : public FTableRowBase
{
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=DungeonUnit)
  FDungeonLogicUnit unitData;
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=DungeonUnit)
  TSubclassOf<ADungeonUnitActor> UnrealActor;
};


inline int FDungeonLogicUnit::getAttackRange()
{
  return attackRange;
}
