#pragma once

#include "CoreMinimal.h"
#include <JsonUtilities/Public/JsonUtilities.h>
#include <Runtime/Core/Public/Algo/Transform.h>

#include "map.h"
#include "Dungeon/DungeonUnitActor.h"
#include "Dungeon/Actions/CombatAction.h"
#include "Dungeon/Actions/EndTurnAction.h"
#include "Dungeon/Actions/MoveAction.h"
#include "DungeonGameState.generated.h"

struct FTurnState
{
  int teamId;
  TSet<int> unitsFinished;
};

struct FUnitInteraction
{
  int unitId;
};

struct FMainMenu
{
  friend bool operator== (const FMainMenu&, const FMainMenu&) { return true; }
};

struct FSelectingUnit
{
};

struct FUnitMenu
{
  int unitId;
};

struct FSelectingUnitAbilityTarget
{
  int unitId;
  int abilityId;
};

enum EActiveWidget
{
  None,
  MainMenu,
  AbilitySelect,
};

struct FMapCursor
{
  FIntPoint Position;
  bool Enabled;
};

struct FBackAction
{
};

template <typename Var1, typename Var2> struct variant_flat;

template <typename ... Ts1, typename ... Ts2>
struct variant_flat<TVariant<Ts1...>, TVariant<Ts2...>>
{
    using type = TVariant<Ts1..., Ts2...>;
};

using TContext = TVariant<
  FSelectingUnit,
  FMainMenu,
  FUnitInteraction,
  FUnitMenu,
  FSelectingUnitAbilityTarget
>;

using TAction = TVariant<
  FEmptyVariantState,
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

  TContext InteractionContext;

  UPROPERTY(VisibleAnywhere)
  FDungeonLogicMap map;

  UPROPERTY(VisibleAnywhere)
  TMap<int, TWeakObjectPtr<ADungeonUnitActor>> unitIdToActor;
};
