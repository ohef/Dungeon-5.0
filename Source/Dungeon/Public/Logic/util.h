#pragma once

#include <CoreMinimal.h>

template <typename FIntPointerHolder = TArray<FIntPoint>>
FIntPointerHolder manhattanReachablePoints(int xLimit, int yLimit, int distance, const FIntPoint& point)
{
  int32 distanceHalved = distance;
  auto xAxis = TRange<int>(point.X - distanceHalved, point.X + distanceHalved);
  auto yAxis = TRange<int>(point.Y - distanceHalved, point.Y + distanceHalved);

  auto widthLimit = TRange<int>(0, xLimit);
  auto heightLimit = TRange<int>(0, yLimit);

  auto restrictedXAxis = TRange<int>::Intersection(xAxis, widthLimit);
  auto restrictedYAxis = TRange<int>::Intersection(yAxis, heightLimit);

  FIntPointerHolder toReturn;

  for (int i = restrictedXAxis.GetLowerBoundValue(); i <= restrictedXAxis.GetUpperBoundValue(); i++)
    for (int j = restrictedYAxis.GetLowerBoundValue(); j <= restrictedYAxis.GetUpperBoundValue(); j++)
    {
      FIntPoint currentPoint = {i, j};
      FIntPoint vectorPoint = FIntPoint(point) - currentPoint;
      bool withinManhattanRange = FGenericPlatformMath::Abs(vectorPoint.X) + FGenericPlatformMath::Abs(vectorPoint.Y) <=
        distanceHalved;
      if (withinManhattanRange)
      {
        toReturn.Add(FIntPoint(i, j));
      }
    }

  return toReturn;
}

template <typename FIntPointerHolder = TArray<FIntPoint>>
FIntPointerHolder pointsInSquareInclusive(const FIntPoint& offset, int xLimit, int yLimit)
{
  FIntPointerHolder toReturn;
  for (int32 i = offset.X; i <= xLimit + offset.X; i++)
  {
    for (int32 j = offset.Y; j <= yLimit + offset.Y; j++)
    {
      toReturn.Add({i, j});
    }
  }
  return toReturn;
}

template <typename T>
struct TIsSmartPointer
{
  static constexpr bool Value = false;
};

template <typename T>
struct TIsSmartPointer<TSharedRef<T>>
{
  static constexpr bool Value = true;
};

template <typename T>
struct TIsSmartPointer<TSharedPtr<T>>
{
  static constexpr bool Value = true;
};

template <typename T>
TTuple<int, T> convertToIdTuple(const T& value)
{
  if constexpr (TOr<TIsPointer<T>, TIsSmartPointer<T>>::Value)
  {
    return TTuple<int, T>(value->id, *value);
  }
  else
  {
    return TTuple<int, T>(value.id, value);
  }
}
