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

  FDungeonLogicUnit(): Id(0), damage(0), HitPoints(0), state(), Movement(0), teamId(0), attackRange(0)
  {
  }

  FDungeonLogicUnit(const FDungeonLogicUnit& Other)
    : Id(Other.Id),
      damage(Other.damage),
      HitPoints(Other.HitPoints),
      state(Other.state),
      Name(Other.Name),
      Movement(Other.Movement),
      teamId(Other.teamId),
      attackRange(Other.attackRange)
  {
  }

  FDungeonLogicUnit& operator=(const FDungeonLogicUnit& Other)
  {
    if (this == &Other)
      return *this;
    Id = Other.Id;
    damage = Other.damage;
    HitPoints = Other.HitPoints;
    state = Other.state;
    Name = Other.Name;
    Movement = Other.Movement;
    teamId = Other.teamId;
    attackRange = Other.attackRange;
    return *this;
  }

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=DungeonUnit)
  int Id;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=DungeonUnit)
  int damage;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=DungeonUnit)
  int HitPoints;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=DungeonUnit)
  TEnumAsByte<UnitState> state;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=DungeonUnit)
  FString Name;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=DungeonUnit)
  int Movement;

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
