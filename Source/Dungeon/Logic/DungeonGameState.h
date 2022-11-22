#pragma once

#include "CoreMinimal.h"
#include <JsonUtilities/Public/JsonUtilities.h>

#include "lager/detail/nodes.hpp"

#include "map.h"
#include "TargetsAvailableId.h"
#include "Actions/InteractAction.hpp"
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

using TStepAction = TVariant<
  FEmptyVariantState,
  FMoveAction,
  FCombatAction
>;

struct FInteractAction
{
	TStepAction mainAction;
	TArray<TInteractionContext> interactions;
};

struct FTargetSubmission
{
  FIntPoint target;
};

using TDungeonAction = TVariant<
  FEmptyVariantState,
  FInteractAction,
  FTargetSubmission,
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

  TStepAction WaitingForResolution;
  TArray<TInteractionContext> InteractionsToResolve;
  TInteractionContext InteractionContext;
  
  FIntPoint CursorPosition;

  UPROPERTY(VisibleAnywhere)
  FDungeonLogicMap map;

  UPROPERTY(VisibleAnywhere)
  TMap<int, TWeakObjectPtr<ADungeonUnitActor>> unitIdToActor;
};