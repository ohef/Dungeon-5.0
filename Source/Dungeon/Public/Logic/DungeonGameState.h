#pragma once

#include "CoreMinimal.h"
#include <Dungeon/Public/Logic/map.h>
#include <JsonUtilities/Public/JsonUtilities.h>
#include <Runtime/Core/Public/Algo/Transform.h>

#include "Dungeon/DungeonUnitActor.h"
#include "DungeonGameState.generated.h"

struct TurnState
{
  int teamId;
  TSet<int> unitsFinished;
};


USTRUCT()
struct FDungeonLogicGameState
{
  GENERATED_BODY()

  TurnState turnState;
  
  UPROPERTY(VisibleAnywhere)
  FDungeonLogicMap map;
  
  UPROPERTY(VisibleAnywhere)
  TMap<int, TWeakObjectPtr<ADungeonUnitActor>> unitIdToActor;
};
