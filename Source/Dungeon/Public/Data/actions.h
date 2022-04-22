#pragma once

#include "CoreMinimal.h"

#include <Dungeon/Public/Logic/map.h>
#include <Dungeon/Private/Utility/Visitor.hpp>

struct CommitAction;
struct MovementAction;
struct WaitAction;

typedef WaitAction WaitRecord;
typedef CommitAction CommitRecord;
struct MovementRecord;

typedef ReducerVisitor<bool, WaitAction, MovementAction, CommitAction> ActionVisitor;
typedef ReducerVisitor<void, WaitRecord, CommitRecord, MovementRecord> RecordVisitor;

struct InteractionInterface
{
  virtual ~InteractionInterface() = default;
  virtual void consumeTarget(FIntPoint) {};
  virtual void consumeUnit(FDungeonLogicUnit) {};
  virtual bool readyForDispatch(){return true;};
};

struct MovementAction : TPayloadAccept<MovementAction, bool, ActionVisitor>, InteractionInterface {
  FIntPoint to;
  int unitID;
};

struct WaitAction : TPayloadAccept<WaitAction, bool, ActionVisitor>, InteractionInterface {
  int unitID;
};

struct CommitAction : TPayloadAccept<CommitAction , bool, ActionVisitor>, InteractionInterface {
  int unitID;
};