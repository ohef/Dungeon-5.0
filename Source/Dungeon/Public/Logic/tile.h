#pragma once

#include <CoreMinimal.h>

#include <tile.generated.h>

USTRUCT()
struct FDungeonLogicTile {
  GENERATED_BODY();

  UPROPERTY() int id;
  UPROPERTY() FString name;
  UPROPERTY() int cost;
};
