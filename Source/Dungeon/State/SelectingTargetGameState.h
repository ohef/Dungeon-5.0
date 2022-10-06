#pragma once
#include "State.h"

class ADungeonGameModeBase;

struct FSelectingTargetGameState : public FState
{
  ADungeonGameModeBase& gameMode;
  TFunction<void(FIntPoint)> handleQuery;
  FDelegateHandle DelegateHandle;

  FSelectingTargetGameState(ADungeonGameModeBase& GameModeBase,
                            TFunction<void(FIntPoint)>&& QueryHandlerFunction
  ) : gameMode(GameModeBase),
      handleQuery(QueryHandlerFunction)
  {
  }

  virtual void Enter() override
  {
    DelegateHandle = gameMode.GetMapCursorPawn()->QueryInput.AddLambda(handleQuery);
  }

  virtual void Exit() override
  {
    gameMode.GetMapCursorPawn()->QueryInput.Remove(DelegateHandle);
  }
};
