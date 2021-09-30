// Copyright Epic Games, Inc. All Rights Reserved.


#include "DungeonGameModeBase.h"

#include <Logic/game.h>
#include <Data/actions.h>
#include "GraphAStar.h"
#include "Actor/MapCursorPawn.h"
#include "Components/TileVisualizationComponent.h"
#include "Curves/CurveVector.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Logic/SimpleTileGraph.h"

class FActionDistpatcher : public ActionVisitor
{
	ADungeonGameModeBase* gameModeBase;

public:
	FActionDistpatcher(ADungeonGameModeBase* gameModeBase) : gameModeBase(gameModeBase)
	{
	}

	FDungeonLogicMap Visit(const MovementAction& ability) const override
	{
		auto& state = gameModeBase->Game->map;
		auto keyPtr = state.unitAssignment.FindKey(ability.unitID);
		if (keyPtr == nullptr) return state;
		auto key = *keyPtr;
		state.unitAssignment.Remove(key);
		state.unitAssignment.Add(ability.to, ability.unitID);
		return state;
	}

	FDungeonLogicMap Visit(const WaitAction& ability) const override
	{
		auto& state = gameModeBase->Game->map;
		return state;
	}

	FDungeonLogicMap Visit(const CommitAction& ability) const override
	{
		auto& state = gameModeBase->Game->map;
		return state;
	}
};

class FRecordEnqueuer : public RecordVisitor
{
	friend ADungeonGameModeBase;
public:
	virtual void Visit(const MovementRecord& action) const override
	{
	}

	virtual void Visit(const CommitRecord& action) const override
	{
	}

	virtual void Visit(const WaitRecord& action) const override
	{
	}
};

void ADungeonGameModeBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	Game = NewObject<UDungeonLogicGame>();
	Game->Init();
}

void ADungeonGameModeBase::MoveUnit(FIntPoint point, int unitID)
{
	MovementAction mab;
	mab.unitID = unitID;
	mab.to = point;
	FActionDistpatcher arv = FActionDistpatcher(this);
	arv.Visit(mab);
}

void ADungeonGameModeBase::Tick(float deltaTime)
{
	Super::Tick(deltaTime);

	if (this->animationQueue.IsEmpty())
		return;

	(*this->animationQueue.Peek())->TickTimeline(deltaTime);
}

ADungeonGameModeBase::ADungeonGameModeBase(const FObjectInitializer& ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
}

void ADungeonGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	for (auto tuple : Game->map.unitAssignment)
	{
		AActor* unitActor = GetWorld()->SpawnActor<AActor>(UnitActorPrefab, FVector::ZeroVector, FRotator::ZeroRotator);
		Game->unitIdToActor.Add(tuple.Value, unitActor);
		unitActor->SetActorLocation(TilePositionToWorldPoint(tuple.Key));
	}

	// FSimpleTileGraph gridGraph = FSimpleTileGraph(this->Game->map);
	// FGraphAStar<FSimpleTileGraph> graph = FGraphAStar<FSimpleTileGraph>(gridGraph);
	// TArray<FIntPoint> outPath;
 //  EGraphAStarResult result = graph.FindPath(FSimpleTileGraph::FNodeRef{ 0, 0 }, FSimpleTileGraph::FNodeRef{ 5, 3 }, gridGraph, outPath);
	// for (int i = 0, j = 1; j < outPath.Num(); ++i, ++j)
	// {
	// 	SubmitLinearAnimation(Game->unitIdToActor.FindChecked(1), outPath[i], outPath[j], 0.25);
	// }

	AActor* tileShowPrefab = GetWorld()->SpawnActor<AActor>(TileShowPrefab, FVector::ZeroVector, FRotator::ZeroRotator);
	UTileVisualizationComponent* component = static_cast<UTileVisualizationComponent*>(tileShowPrefab->
		GetComponentByClass(UTileVisualizationComponent::StaticClass()));

	auto mapPawn = static_cast<AMapCursorPawn*>(GetWorld()->GetFirstPlayerController()->GetPawn());
	mapPawn->CursorEvent.AddLambda([&, component](FIntPoint pt)
	{
		UKismetSystemLibrary::PrintString(this, pt.ToString(), true, false);
		FDungeonLogicMap DungeonLogicMap = Game->map;
		TMap<FIntPoint, int> Tuples = DungeonLogicMap.unitAssignment;
		if (Tuples.Contains(pt))
		{
			auto DungeonLogicUnit = DungeonLogicMap.loadedUnits.FindChecked(Tuples.FindChecked(pt));
			TArray<FIntPoint> IntPoints = manhattanReachablePoints(pt, DungeonLogicMap.Width, DungeonLogicMap.Height, DungeonLogicUnit.movement * 2);
			component->ShowTiles(IntPoints );
		}
		else
		{
			if (component->GetInstanceCount() > 0)
				component->Clear();
		}
	});
}
