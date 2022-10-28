#pragma once

#include <CoreMinimal.h>

#include "unit.h"
#include "tile.h"

#include "map.generated.h"

USTRUCT(BlueprintType)
struct FDungeonLogicMap
{
  GENERATED_BODY()

  UPROPERTY(Category=Dungeon, VisibleAnywhere)
  int Width;
  UPROPERTY(Category=Dungeon, VisibleAnywhere)
  int Height;

  UPROPERTY(Category=Dungeon, VisibleAnywhere)
  TMap<int, FDungeonLogicUnit> loadedUnits;
  UPROPERTY(Category=Dungeon, VisibleAnywhere)
  TMap<int, FDungeonLogicTile> loadedTiles;

  UPROPERTY(Category=Dungeon, VisibleAnywhere)
  TMap<FIntPoint, int> tileAssignment;
  UPROPERTY(Category=Dungeon, VisibleAnywhere)
  TMap<FIntPoint, int> unitAssignment;

  // boost::bimap<FIntPoint, int> tileAssignmentt;
};

// auto wew = []()
// {
//   boost::bimap<FIntPoint, int> tileAssignmentt;
//   auto wow = FDungeonLogicMap();
//   wow.tileAssignmentt.find(FIntPoint{1, 2});
//   auto wowthatscring = wow.tileAssignmentt.right[1];
// };
