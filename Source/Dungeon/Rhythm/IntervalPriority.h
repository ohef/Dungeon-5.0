#pragma once

struct FIntervalPriority 
{
  int8 order;
  float hitTime;
  FFloatInterval interval;

  FIntervalPriority(float InMin, float InMax, int8 order)
    : order(order), hitTime(-1), interval(InMin, InMax)
  {
  }
};

using FInteractionResults = TArray<FIntervalPriority>;