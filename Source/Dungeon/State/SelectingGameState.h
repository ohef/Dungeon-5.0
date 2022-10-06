#pragma once

#include "State.h"
#include "Dungeon/TargetsAvailableId.h"
#include "Logic/unit.h"

class ADungeonGameModeBase;

struct FSelectingGameState : public FState
{
  ADungeonGameModeBase& gameMode;
  FDungeonLogicUnit* foundUnit;
  TMap<ETargetsAvailableId, TSet<FIntPoint>> targets;
  TFunction<void()> unregisterDelegates;

  explicit FSelectingGameState(ADungeonGameModeBase& GameMode);

  void OnCursorPositionUpdate(FIntPoint);
  void OnCursorQuery(FIntPoint);

  virtual void Enter() override;
  virtual void Exit() override;
};
