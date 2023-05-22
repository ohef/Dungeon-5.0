// Fill out your copyright notice in the Description page of Project Settings.


#include "MapBounds.h"

#include "DungeonConstants.h"
#include "Components/BoxComponent.h"


// Sets default values
AMapBounds::AMapBounds()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>("Scene");

	for (auto direction : {FVector::ForwardVector, FVector::BackwardVector})
	{
		auto scaleDirection = 100;
		auto boxExtentDirection = FVector(10, 0, 10) + FVector::RightVector * 0.5 * scaleDirection * TILE_POINT_SCALE;

		auto directionPosition = direction * 0.5 * scaleDirection * TILE_POINT_SCALE;
		// + direction * .5 * TILE_POINT_SCALE;
		auto positionBoxCollider = CreateDefaultSubobject<UBoxComponent>(FName(direction.ToString()));
		positionBoxCollider->SetWorldLocation(directionPosition);
		positionBoxCollider->SetBoxExtent(boxExtentDirection);
		positionBoxCollider->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	}

	for (auto direction : {FVector::LeftVector, FVector::RightVector})
	{
		auto scaleDirection = 100;
		auto boxExtentDirection = FVector(0, 10, 10) + FVector::ForwardVector * 0.5 * scaleDirection * TILE_POINT_SCALE;

		auto directionPosition = direction * 0.5 * scaleDirection * TILE_POINT_SCALE;
		// + direction * .5 * TILE_POINT_SCALE;
		auto positionBoxCollider = CreateDefaultSubobject<UBoxComponent>(FName(direction.ToString()));
		positionBoxCollider->SetWorldLocation(directionPosition);
		positionBoxCollider->SetBoxExtent(boxExtentDirection);
		positionBoxCollider->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	}
}

// Called when the game starts or when spawned
void AMapBounds::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void AMapBounds::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
