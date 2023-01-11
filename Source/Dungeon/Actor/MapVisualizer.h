// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MapVisualizer.generated.h"

UCLASS()
class DUNGEON_API AMapVisualizer : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AMapVisualizer();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
