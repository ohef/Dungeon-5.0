#pragma once

#include "CoreMinimal.h"

#include <Dungeon/Public/Logic/map.h>

template<typename ReturnType, typename...>
class ReducerVisitor {
public:
  virtual ~ReducerVisitor() {};
  void Visit() {};
};

template<typename ReturnType, typename A, typename... ActionType>
class ReducerVisitor<ReturnType, A, ActionType...> : ReducerVisitor<ReturnType,ActionType...> {
public:
  using ReducerVisitor<ReturnType,ActionType...>::Visit;

  virtual ReturnType Visit(const A& action) const = 0;
};

struct CommitAction;
struct MovementAction;
struct WaitAction;

typedef WaitAction WaitRecord;
typedef CommitAction CommitRecord;
struct MovementRecord;

typedef ReducerVisitor<FDungeonLogicMap, WaitAction, MovementAction, CommitAction> ActionVisitor;
typedef ReducerVisitor<void, WaitRecord, CommitRecord, MovementRecord> RecordVisitor;

template<typename Derived, typename ReturnType>
struct PayloadAccept {
public:
  virtual ReturnType Accept(const ActionVisitor& visitor) {
    return visitor.Visit(*static_cast<Derived*>(this));
  };
};

struct MovementAction : public PayloadAccept<MovementAction, FDungeonLogicMap>  {
public:
  FIntPoint to;
  int unitID;
};

struct WaitAction : public PayloadAccept<WaitAction, FDungeonLogicMap> {
public:
  int unitID;
};

struct CommitAction : public PayloadAccept<CommitAction , FDungeonLogicMap> {
public:
  int unitID;
};

struct MovementRecord : public MovementAction {
public:
  FIntPoint from;
};
