#pragma once

#include "CoreMinimal.h"
#include "map.h"

class DUNGEON_API FSimpleTileGraph
{
  const FDungeonLogicMap& Map;
  FIntPoint GDirections[4] = {
    {1, 0},
    {0, 1},
    {-1, 0},
    {0, -1},
  };

public:
  typedef FIntPoint FNodeRef;

  TSet<FNodeRef> invalidMask;
  int32 maxPathLength;

  FSimpleTileGraph(const FDungeonLogicMap& Map, int32 MaxPathLength = 99999,
                   const TSet<FNodeRef>& InvalidMask = TSet<FNodeRef>())
    : Map(Map),
      invalidMask(InvalidMask),
      maxPathLength(MaxPathLength)
  {
  }

  int32 GetNeighbourCount(FNodeRef NodeRef) const
  {
    return 4;
  };

  bool IsValidRef(FNodeRef NodeRef) const
  {
    return Map.tileAssignment.Contains(NodeRef) && !invalidMask.Contains(NodeRef);
  };

  FNodeRef GetNeighbour(const FNodeRef NodeRef, const int32 NeighbourIndex) const
  {
    return FNodeRef(NodeRef) + GDirections[NeighbourIndex];
  };

  float GetHeuristicScale() const
  {
    return 1.0;
  }

  float GetHeuristicCost(const FNodeRef StartNodeRef, const FNodeRef EndNodeRef) const
  {
    return (FNodeRef(EndNodeRef) - StartNodeRef).Size();
  }

  float GetTraversalCost(const FNodeRef StartNodeRef, const FNodeRef EndNodeRef) const
  {
    return 1.0;
  }

  bool IsTraversalAllowed(const FNodeRef NodeA, const FNodeRef NodeB) const
  {
    return true;
  }

  int32 GetMaxSearchNodes(uint32 NodeCount) const
  {
    return maxPathLength;
  }

  bool WantsPartialSolution() const
  {
    return true;
  }
};
