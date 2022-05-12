#pragma once

#include <CoreMinimal.h>
#include <Data/actions.h>

#include "Engine/DataTable.h"
#include "Unit.generated.h"

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
  
  // TArray<TWeakPtr<FDungeonAbility>> abilities;
};

inline int FDungeonLogicUnit::getAttackRange()
{
  return attackRange;
}
