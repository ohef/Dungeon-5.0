// Fill out your copyright notice in the Description page of Project Settings.


#include "DungeonConstants.h"

FVector TilePositionToWorldPoint(const FIntPoint& point)
{
  return FVector(point) * TILE_POINT_SCALE;
}

#define CONSTANT_STRING(x) FName G ## x = #x

CONSTANT_STRING(MoveRight);
CONSTANT_STRING(MoveUp);
CONSTANT_STRING(CameraRotate);
CONSTANT_STRING(Query);
CONSTANT_STRING(KeyFocus);
