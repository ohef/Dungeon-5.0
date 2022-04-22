#pragma once

#include "CoreMinimal.h"
#include <Dungeon/Public/Logic/unit.h>
#include <Dungeon/Public/Logic/tile.h>
#include <Dungeon/Public/Logic/util.h>
#include <Dungeon/Public/Logic/map.h>
#include <JsonUtilities/Public/JsonUtilities.h>
#include <Runtime/Core/Public/Algo/Transform.h>

#include "Dungeon/DungeonUnitActor.h"
#include "game.generated.h"

UCLASS()
class UDungeonLogicGame : public UObject
{
  GENERATED_BODY()

public:
  UPROPERTY(VisibleAnywhere)
  FDungeonLogicMap map;

  UPROPERTY(VisibleAnywhere)
  TMap<int, TWeakObjectPtr<ADungeonUnitActor>> unitIdToActor;

  UPROPERTY(VisibleAnywhere)
  FIntPoint cursorLocation;

  UDungeonLogicGame()
  {
    map.Width = 10;
    map.Height = 10;
  }

  void Init()
  {
    auto zaunit = MakeShared<FDungeonLogicUnit>();
    zaunit->id = 1;
    zaunit->name = TEXT("Test");
    zaunit->movement = 3;

    auto zaTile = MakeShared<FDungeonLogicTile>();
    zaTile->id = 1;
    zaTile->name = TEXT("Grass");
    zaTile->cost = 1;

    map.unitAssignment.Add({1, 10}, 1);

    TArray<FIntPoint> points = pointsInSquareInclusive({0, 0}, map.Width, map.Height);
    Algo::Transform(points, this->map.tileAssignment, [&zaTile](FIntPoint p)
    {
      return TTuple<FIntPoint, int>{p, zaTile->id};
    });

    map.loadedUnits.Add(convertToIdTuple(*zaunit));
    map.loadedTiles.Add(convertToIdTuple(*zaTile));
  }
};
