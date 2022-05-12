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

  virtual void Visit(const A& action) const = 0;
};

template <typename TVisitor>
struct TPayloadAccept
{
  virtual ~TPayloadAccept() = default;

  virtual void Accept(const TVisitor& visitor) = 0;
};
