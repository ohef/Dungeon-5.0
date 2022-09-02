#pragma once

#include "CoreMinimal.h"
#include "Logic/Visitor.h"

struct FCommitAction;
struct FMovementAction;
struct FWaitAction;
struct FAttackAction;

typedef ReducerVisitor<FAttackAction, FWaitAction, FMovementAction, FCommitAction> ActionVisitor;

template<typename TDownCast>
struct FDungeonAbility : public TPayloadAccept<ActionVisitor, TDownCast>
{
  FName Name;
};

struct FMovementAction : public FDungeonAbility<FMovementAction>
{
  FIntPoint to;
};

struct FAttackAction : public FDungeonAbility<FMovementAction>
{
  int initialPower;
};

struct FWaitAction : public FDungeonAbility<FWaitAction>
{
};

struct FCommitAction : public FDungeonAbility<FCommitAction> 
{
};