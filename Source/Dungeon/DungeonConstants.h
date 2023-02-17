// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

const int TILE_POINT_SCALE = 100;

inline FVector TilePositionToWorldPoint(const FIntPoint& point)
{
  return FVector(point) * TILE_POINT_SCALE;
}

#define CONSTANT_STRING_EXTERNAL(x) const FName G ## x = #x ;

CONSTANT_STRING_EXTERNAL(MoveRight);
CONSTANT_STRING_EXTERNAL(MoveUp);
CONSTANT_STRING_EXTERNAL(CameraRotate);
CONSTANT_STRING_EXTERNAL(Query);
CONSTANT_STRING_EXTERNAL(GoBack);
CONSTANT_STRING_EXTERNAL(KeyFocus);
CONSTANT_STRING_EXTERNAL(Zoom);
CONSTANT_STRING_EXTERNAL(OpenMenu);
