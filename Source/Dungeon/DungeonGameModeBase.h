// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"

#include <Engine/Classes/Components/TimelineComponent.h>
#include <Dungeon/ThirdParty/flowcpp/flow.h>
#include <Logic/game.h>

#include "Curves/CurveVector.h"
#include "DungeonGameModeBase.generated.h"


/**
 * 
 */
UCLASS()
class DUNGEON_API ADungeonGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

	ADungeonGameModeBase(const FObjectInitializer& ObjectInitializer);

public:
	virtual void BeginPlay() override;
	virtual void Tick(float time) override;

	TQueue<TUniquePtr<FTimeline>> animationQueue;

	int currentPlayer;

	UFUNCTION(BlueprintCallable)
	void MoveUnit(FIntPoint point, int unitID);

	UPROPERTY(EditAnywhere)
	UClass* TileShowPrefab;
	UPROPERTY(EditAnywhere)
	UDungeonLogicGame* Game;
	UPROPERTY(EditAnywhere)
	UClass* UnitActorPrefab;
	UPROPERTY(EditAnywhere)
	UStaticMesh* UnitMeshIndicator;
	UPROPERTY(EditAnywhere)
	float MovementAnimationSpeedInSeconds;
	UPROPERTY(EditAnywhere)
	UMaterialInstance* UnitCommitMaterial;

	virtual void PostInitializeComponents() override;

template<typename  T>
void SubmitLinearAnimation(T* movable, FIntPoint from, FIntPoint to, float time)
{
	auto timeline = MakeUnique<FTimeline>();
	FOnTimelineVectorStatic onUpdate;
	FOnTimelineEventStatic onFinish;
	UCurveVector* curve = NewObject<UCurveVector>();
	from = from * 100;
	to = to * 100;
	curve->FloatCurves[0].AddKey(0.0, from.X);
	curve->FloatCurves[0].AddKey(time, to.X);
	curve->FloatCurves[1].AddKey(0.0, from.Y);
	curve->FloatCurves[1].AddKey(time, to.Y);

	if constexpr (TIsDerivedFrom<T, AActor>::Value)
	{
		onUpdate.BindLambda([movable](FVector interp)
		{
			movable->SetActorLocation(interp, false);
		});
	} else
	{
		onUpdate.BindLambda([movable](FVector interp)
		{
			movable->SetWorldLocation(interp, false);
		});
	}
	
	onFinish.BindLambda([this]()
	{
		this->animationQueue.Pop();
	});

	timeline->SetTimelineFinishedFunc(onFinish);
	timeline->AddInterpVector(curve, onUpdate);
	timeline->Play();

	this->animationQueue.Enqueue(MoveTemp(timeline));
}
	
};
