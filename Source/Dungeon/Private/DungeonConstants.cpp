// Fill out your copyright notice in the Description page of Project Settings.


#include "DungeonConstants.h"

FVector TilePositionToWorldPoint(const FIntPoint& point)
{
  return FVector(point) * TILE_POINT_SCALE;
}
