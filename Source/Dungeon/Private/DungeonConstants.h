// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

const int TILE_POINT_SCALE = 100;

FVector TilePositionToWorldPoint(const FIntPoint& point);

#define CONSTANT_STRING_EXTERNAL(x) extern FName G ## x 

CONSTANT_STRING_EXTERNAL(MoveRight);
CONSTANT_STRING_EXTERNAL(MoveUp);
CONSTANT_STRING_EXTERNAL(CameraRotate);
CONSTANT_STRING_EXTERNAL(Query);
CONSTANT_STRING_EXTERNAL(KeyFocus);