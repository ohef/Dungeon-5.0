#pragma once

#include "CoreMinimal.h"
#include "Logic/Visitor.h"

struct FCommitAction;
struct FMovementAction;
struct FWaitAction;
struct FAttackAction;

typedef ReducerVisitor<FAttackAction, FWaitAction, FMovementAction, FCommitAction> ActionVisitor;

#define AcceptFunction(VisitorName) virtual void Accept(const VisitorName& visitor) { visitor.Visit(*this); }

struct FDungeonAbility : public TPayloadAccept<ActionVisitor>
{
  FName Name;
};

struct FMovementAction : public FDungeonAbility
{
  FIntPoint to;
  
  AcceptFunction(ActionVisitor)
};

struct FAttackAction : public FDungeonAbility
{
  int initialPower;
  
  AcceptFunction(ActionVisitor)
};

struct FWaitAction : public FDungeonAbility
{
  AcceptFunction(ActionVisitor)
};

struct FCommitAction : public FDungeonAbility
{
  AcceptFunction(ActionVisitor)
};