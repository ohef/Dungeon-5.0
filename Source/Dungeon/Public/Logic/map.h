#pragma once

#include <CoreMinimal.h>

#include "unit.h"
#include "tile.h"

#include "map.generated.h"

typedef int TileID;
typedef int UnitID;

USTRUCT(BlueprintType)
struct FDungeonLogicMap
{
  GENERATED_BODY()

  UPROPERTY(Category=Dungeon, VisibleAnywhere) int Width;
  UPROPERTY(Category=Dungeon, VisibleAnywhere) int Height;
                              
  UPROPERTY(Category=Dungeon, VisibleAnywhere) TMap<int, FDungeonLogicUnit> loadedUnits;
  UPROPERTY(Category=Dungeon, VisibleAnywhere) TMap<int, FDungeonLogicTile> loadedTiles;
                              
  UPROPERTY(Category=Dungeon, VisibleAnywhere) TMap<FIntPoint, int> tileAssignment;
  UPROPERTY(Category=Dungeon, VisibleAnywhere) TMap<FIntPoint, int> unitAssignment;
};

