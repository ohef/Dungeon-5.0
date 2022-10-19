#pragma once

#include "CoreMinimal.h"
#include <Dungeon/Public/Logic/map.h>
#include <JsonUtilities/Public/JsonUtilities.h>
#include <Runtime/Core/Public/Algo/Transform.h>

#include "Dungeon/DungeonUnitActor.h"
#include "DungeonGameState.generated.h"

struct FTurnState
{
  int teamId;
  TSet<int> unitsFinished;
};

enum Wow {
  Dude,Thats,Crazy
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

using TContext = TVariant<
  FSelectingUnit,
  FMainMenu,
  FUnitInteraction,
  FUnitMenu,
  FSelectingUnitAbilityTarget
>;

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

using TDungeonAction = TVariant<
  FEmptyVariantState,
  FBackAction
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
