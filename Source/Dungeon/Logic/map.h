#pragma once

#include <CoreMinimal.h>

#include "unit.h"
#include "tile.h"

#include "map.generated.h"

USTRUCT(BlueprintType)
struct FDungeonLogicMap
{
  GENERATED_BODY()

  UPROPERTY(Category=Dungeon, EditAnywhere)
  int Width;
  UPROPERTY(Category=Dungeon, EditAnywhere)
  int Height;

  UPROPERTY(Category=Dungeon, EditAnywhere)
  TMap<int, FName> AbilityOwnership;
  UPROPERTY(Category=Dungeon, EditAnywhere)
  TMap<int, FDungeonLogicUnit> LoadedUnits;
  UPROPERTY(Category=Dungeon, EditAnywhere)
  TMap<int, FDungeonLogicTile> LoadedTiles;

  UPROPERTY(Category=Dungeon, EditAnywhere)
  TMap<FIntPoint, int> TileAssignment;
  UPROPERTY(Category=Dungeon, EditAnywhere)
  TMap<FIntPoint, int> UnitAssignment;
};