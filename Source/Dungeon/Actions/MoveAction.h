#pragma once

#include "MoveAction.generated.h"

USTRUCT()
struct FMoveAction
{
  GENERATED_BODY()
  
  int InitiatorId;
  FIntPoint Destination;

  FMoveAction(int ID, const FIntPoint& Destination)
    : InitiatorId(ID),
      Destination(Destination)
  {
  }

  FMoveAction(int InitiatorId)
    : FMoveAction(InitiatorId, {0, 0})
  {
  }

  FMoveAction() : FMoveAction(0) {
  }
  
};

