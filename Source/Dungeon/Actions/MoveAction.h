#pragma once
#include "Action.h"

struct FMoveAction : public FAction
{
  int InitiatorId;
  FIntPoint Destination;

  FMoveAction(int ID, const FIntPoint& Destination)
    : InitiatorId(ID),
      Destination(Destination)
  {
  }
};

