#pragma once

template <typename...>
class ReducerVisitor
{
public:
  virtual ~ReducerVisitor()
  {
  };

  void Visit()
  {
  };
};

template <typename A, typename... ActionType>
class ReducerVisitor<A, ActionType...> : ReducerVisitor<ActionType...>
{
public:
  using ReducerVisitor<ActionType...>::Visit;

  virtual void Visit(const A& action) const {};
};

template <typename TVisitor, typename TAcceptorType>
struct TPayloadAccept
{
  virtual ~TPayloadAccept() = default;

  void Accept(const TVisitor& visitor)
  {
    visitor.Visit(static_cast<TAcceptorType>(*this));
  };
};
