#pragma once

struct FState
{
  virtual ~FState() = default;

  virtual void Enter()
  {
  };

  virtual void Exit()
  {
  };
};
