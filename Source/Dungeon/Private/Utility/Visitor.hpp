#pragma once

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

template<typename TDerived, typename TReturnType, typename TVisitor>
struct TPayloadAccept {
  virtual ~TPayloadAccept() = default;

  virtual TReturnType Accept(const TVisitor& visitor) {
    return visitor.Visit(*static_cast<TDerived*>(this));
  };
};