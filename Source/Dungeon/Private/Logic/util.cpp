#include <Dungeon/Public/Logic/util.h>

TArray<FIntPoint> manhattanReachablePoints(const FIntPoint& point, int xLimit, int yLimit, int distance)
{
  int32 distanceHalved = distance / 2;
  auto xAxis = TRange<int>(point.X - distanceHalved, point.X + distanceHalved);
  auto yAxis = TRange<int>(point.Y - distanceHalved, point.Y + distanceHalved);

  auto widthLimit = TRange<int>(0, xLimit);
  auto heightLimit = TRange<int>(0, yLimit);

  auto restrictedXAxis = TRange<int>::Intersection(xAxis, widthLimit);
  auto restrictedYAxis = TRange<int>::Intersection(yAxis, heightLimit);

  TArray<FIntPoint> toReturn;

  for (int i = restrictedXAxis.GetLowerBoundValue(); i <= restrictedXAxis.GetUpperBoundValue(); i++)
    for (int j = restrictedYAxis.GetLowerBoundValue(); j <= restrictedYAxis.GetUpperBoundValue(); j++)
    {
      FIntPoint currentPoint = {i,j};
      FIntPoint vectorPoint = FIntPoint(point) - currentPoint;
      bool withinManhattanRange = FGenericPlatformMath::Abs(vectorPoint.X) + FGenericPlatformMath::Abs(vectorPoint.Y) <= distanceHalved;
      if (withinManhattanRange )
      {
        toReturn.Add(FIntPoint(i, j));
      }
    }

  return toReturn;
}

TArray<FIntPoint> pointsInSquareInclusive(const FIntPoint& offset, int xLimit, int yLimit)
{
  TArray<FIntPoint> toReturn;
  for (int32 i = offset.X; i <= xLimit + offset.X; i++)
  {
    for (int32 j = offset.Y; j <= yLimit + offset.Y; j++)
    {
      toReturn.Add({ i,j });
    }
  }
  return toReturn;
}
