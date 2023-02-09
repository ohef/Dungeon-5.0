#pragma once

#include "CoreMinimal.h"

#include "map.h"
#include "TargetsAvailableId.h"
#include "Dungeon/Actions/CombatAction.h"
#include "Dungeon/Actions/EndTurnAction.h"
#include "Dungeon/Actions/MoveAction.h"
#include "Rhythm/IntervalPriority.h"
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
	friend bool operator==(const FUnitInteraction& Lhs, const FUnitInteraction& RHS)
	{
		return Lhs.targetIDUnderFocus == RHS.targetIDUnderFocus
			&& Lhs.originatorID == RHS.originatorID;
	}

	friend bool operator!=(const FUnitInteraction& Lhs, const FUnitInteraction& RHS)
	{
		return !(Lhs == RHS);
	}

	GENERATED_BODY()

	int targetIDUnderFocus;
	int originatorID;
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
	TSet<FName> deactivatedAbilities;
	FName focusedAbilityName;
};

enum EAbilityId
{
	IdMove = 1,
	IdAttack,
};

USTRUCT()
struct FSelectingUnitAbilityTarget
{
	GENERATED_BODY()

	EAbilityId abilityId;
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

struct FSteppedAction
{
	TStepAction mainAction;
	TArray<TInteractionContext> interactions;
};

struct FCursorQueryTarget
{
	FIntPoint target;
};

struct FCommitAction
{
};

struct FCheckPoint
{
};

struct FFocusChanged
{
	FName focusName;
};

struct FSpawnUnit
{
	FIntPoint position;
	FDungeonLogicUnit unit;
};

using TDungeonAction = TVariant<
	FEmptyVariantState,
	FSpawnUnit,
	FInteractionResults,
	FSteppedAction,
	FCursorQueryTarget,
	FChangeState,
	FCursorPositionUpdated,
	FMoveAction,
	FCombatAction,
	FEndTurnAction,
	FBackAction,
	FWaitAction,
	FCommitAction,
	FFocusChanged
>;

USTRUCT()
struct FDungeonWorldState
{
	GENERATED_BODY()

	FTurnState TurnState;

	TStepAction WaitingForResolution;
	TArray<TInteractionContext> InteractionsToResolve;
	TInteractionContext InteractionContext;

	UPROPERTY(EditAnywhere)
	FIntPoint CursorPosition;

	UPROPERTY(EditAnywhere)
	FDungeonLogicMap Map;

	UPROPERTY(EditAnywhere)
	TMap<int, TWeakObjectPtr<ADungeonUnitActor>> unitIdToActor;
};
