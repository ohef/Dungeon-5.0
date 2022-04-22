#pragma once

template<typename ReturnType, typename...>
class ReducerVisitor {
public:
  virtual ~ReducerVisitor() {};
  void Visit() {};
};

template<typename TReturnType, typename A, typename... ActionType>
class ReducerVisitor<TReturnType, A, ActionType...> : ReducerVisitor<TReturnType,ActionType...> {
public:
  using ReducerVisitor<TReturnType,ActionType...>::Visit;

  virtual TReturnType Visit(const A& action) const = 0;
};

template<typename TDerived, typename TReturnType, typename TVisitor>
struct TPayloadAccept {
  virtual ~TPayloadAccept() = default;

  virtual TReturnType Accept(const TVisitor& visitor) {
    return visitor.Visit(*static_cast<TDerived*>(this));
  };
};