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

USTRUCT()
struct FTurnState
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere)
	int teamId;
	
	UPROPERTY(EditAnywhere)
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

USTRUCT()
struct FControlledInteraction
{
	GENERATED_BODY()
};

using TInteractionContext = TVariant<
	FControlledInteraction,
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

struct FAttachLogicToDisplayUnit
{
	int Id;
	ADungeonUnitActor* actor;
};

struct FChangeTeam
{
	int newTeamID;
};

struct FSpawnUnit
{
	FIntPoint Position;
	FDungeonLogicUnit Unit;
	FString PrefabClass;
};

using TDungeonAction = TVariant<
	FEmptyVariantState,
	FSpawnUnit,
	FChangeTeam,
	FAttachLogicToDisplayUnit,
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

UENUM()
enum EPlayerType
{
	Player,
	Computer
};

USTRUCT()
struct FConfig
{
	GENERATED_BODY()
	
	TArray<EPlayerType> ControllerTypeMapping;
};

USTRUCT()
struct FDungeonWorldState
{
	GENERATED_BODY()

	TStepAction WaitingForResolution;
	TArray<TInteractionContext> InteractionsToResolve;
	TInteractionContext InteractionContext;
	
	UPROPERTY(EditAnywhere)
	FTurnState TurnState;
	
	UPROPERTY(EditAnywhere)
	FConfig Config;

	UPROPERTY(EditAnywhere)
	FIntPoint CursorPosition;

	UPROPERTY(EditAnywhere)
	FDungeonLogicMap Map;

	UPROPERTY(EditAnywhere)
	TMap<int, TWeakObjectPtr<ADungeonUnitActor>> unitIdToActor;
};
