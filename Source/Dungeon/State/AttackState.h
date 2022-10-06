#pragma once

#include "State.h"
#include "Dungeon/DungeonGameModeBase.h"
#include "Dungeon/SingleSubmitHandler.h"

struct FAttackState : public FState
{
  ADungeonGameModeBase& gameMode;
  TWeakObjectPtr<USingleSubmitHandler> SingleSubmitHandler;

  FAttackState(ADungeonGameModeBase& GameMode, USingleSubmitHandler* SingleSubmitHandler)
    : gameMode(GameMode),
      SingleSubmitHandler(SingleSubmitHandler)
  {
  }

  virtual void Enter() override;

  virtual void Exit() override;
};
