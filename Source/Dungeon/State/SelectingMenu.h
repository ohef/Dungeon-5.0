#pragma once

#include "State.h"
#include "Dungeon/DungeonGameModeBase.h"

struct FSelectingMenu : public FState, TSharedFromThis<FSelectingMenu>
{
  ADungeonGameModeBase& gameMode;
  FIntPoint capturedCursorPosition;
  FDungeonLogicUnit* initiatingUnit;
  TMap<ETargetsAvailableId, TSet<FIntPoint>> targets;

  FSelectingMenu(ADungeonGameModeBase& GameMode, const FIntPoint& CapturedCursorPosition,
                 FDungeonLogicUnit* InitiatingUnit, const TMap<ETargetsAvailableId, TSet<FIntPoint>>& Targets)
    : gameMode(GameMode),
      capturedCursorPosition(CapturedCursorPosition),
      initiatingUnit(InitiatingUnit),
      targets(Targets)
  {
  }

  virtual void Enter() override;

  virtual void Exit() override;

  FReply OnAttackButtonClick();

  FReply OnWaitButtonClick();

  FReply OnMoveSelected();
};
