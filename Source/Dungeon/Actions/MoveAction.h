#pragma once

#include "MoveAction.generated.h"

USTRUCT()
struct FMoveAction
{
	GENERATED_BODY()

	UPROPERTY()
	int InitiatorId;
	UPROPERTY()
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

	FMoveAction() : FMoveAction(0)
	{
	}
};

USTRUCT()
struct FMoveActionCommit : public FMoveAction
{
	GENERATED_BODY()
};
