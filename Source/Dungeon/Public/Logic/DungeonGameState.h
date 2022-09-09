#pragma once

#include "CoreMinimal.h"
#include <Dungeon/Public/Logic/unit.h>
#include <Dungeon/Public/Logic/map.h>
#include <JsonUtilities/Public/JsonUtilities.h>
#include <Runtime/Core/Public/Algo/Transform.h>

#include "Dungeon/DungeonUnitActor.h"
#include "DungeonGameState.generated.h"

struct TurnState
{
  int TeamId;
  TSet<int> unitsFinished;
  TSet<int> freeUnits;
};

UCLASS()
class UDungeonLogicGameState : public UObject
{
  GENERATED_BODY()

public:
  UPROPERTY(VisibleAnywhere)
  FDungeonLogicMap map;

  TurnState turnState;

  UPROPERTY(VisibleAnywhere)
  TMap<int, TWeakObjectPtr<ADungeonUnitActor>> unitIdToActor;
};
