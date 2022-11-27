#pragma once

struct FIntervalPriority : public FFloatInterval
{
  int8 order;
  float hitTime;

  using FFloatInterval::FFloatInterval;

  FIntervalPriority(float InMin, float InMax, int8 order)
    : FFloatInterval(InMin, InMax), order(order)
  {
  }
};

using FInteractionResults = TArray<FIntervalPriority>;
using FInteractionAction = FInteractionResults;
