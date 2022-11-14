#pragma once

#include "CoreMinimal.h"
#include <JsonUtilities/Public/JsonUtilities.h>

#include "lager/detail/nodes.hpp"

#include "map.h"
#include "TargetsAvailableId.h"
#include "Dungeon/DungeonUnitActor.h"
#include "Dungeon/Actions/CombatAction.h"
#include "Dungeon/Actions/EndTurnAction.h"
#include "Dungeon/Actions/MoveAction.h"
#include "DungeonGameState.generated.h"

//vim macro: OUSTRUCT()<C-c>jjoGENERATED_BODY()

using UnitId = int;

struct FTurnState
{
  int teamId;
  TSet<int> unitsFinished;
};

USTRUCT()
struct FUnitInteraction
{
  GENERATED_BODY()
  int unitId;
};

USTRUCT()
struct FMainMenu
{
  GENERATED_BODY()
};

USTRUCT()
struct FSelectingUnitContext
{
  GENERATED_BODY()
  
private:
  auto SetupTMap()
  {
    TMap<ETargetsAvailableId, TSet<FIntPoint>> map;
    map.Add(ETargetsAvailableId::move, TSet<FIntPoint>());
    map.Add(ETargetsAvailableId::attack, TSet<FIntPoint>());
    return map;
  }
  
public:
  FSelectingUnitContext() : unitUnderCursor(TOptional<UnitId>()), interactionTiles(SetupTMap())
  {
  }

  TOptional<UnitId> unitUnderCursor;
  TMap<ETargetsAvailableId, TSet<FIntPoint>> interactionTiles;

};

USTRUCT()
struct FUnitMenu
{
  GENERATED_BODY()
  int unitId;
};

USTRUCT()
struct FSelectingUnitAbilityTarget
{
  GENERATED_BODY()
  
  int unitId;
  int abilityId;
};

struct FMapCursor
{
  FIntPoint Position;
  bool Enabled;
};

struct FBackAction
{
};

struct FCursorPositionUpdated
{
  FIntPoint cursorPosition;
};

template <typename Var1, typename Var2>
struct variant_flat;

template <typename ... Ts1, typename ... Ts2>
struct variant_flat<TVariant<Ts1...>, TVariant<Ts2...>>
{
  using type = TVariant<Ts1..., Ts2...>;
};

using TInteractionContext = TVariant<
  FSelectingUnitContext,
  FMainMenu,
  FUnitInteraction,
  FUnitMenu,
  FSelectingUnitAbilityTarget
>;

struct FChangeState
{
  TInteractionContext newState;
};

using TAction = TVariant<
  FEmptyVariantState,
  FChangeState,
  FCursorPositionUpdated,
  FMoveAction,
  FCombatAction,
  FEndTurnAction,
  FBackAction,
  FWaitAction
>;

USTRUCT()
struct FDungeonWorldState
{
  GENERATED_BODY()

  FTurnState TurnState;

  TInteractionContext InteractionContext;
  
  FIntPoint CursorPosition;

  UPROPERTY(VisibleAnywhere)
  FDungeonLogicMap map;

  UPROPERTY(VisibleAnywhere)
  TMap<int, TWeakObjectPtr<ADungeonUnitActor>> unitIdToActor;
};