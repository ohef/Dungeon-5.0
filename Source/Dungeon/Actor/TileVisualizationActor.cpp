// Fill out your copyright notice in the Description page of Project Settings.


#include "Actor/TileVisualizationActor.h"

#include "Lenses/model.hpp"
#include "Dungeon/Utility/HookFunctor.hpp"
#include "Dungeon/DungeonGameModeBase.h"

#define FastSubobject(FieldName) \
FieldName = CreateDefaultSubobject<TRemovePointer<decltype(FieldName)>::Type>(#FieldName);

ATileVisualizationActor::ATileVisualizationActor(const FObjectInitializer& ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;

	FastSubobject(MovementVisualizationComponent)
	FastSubobject(TileVisualizationComponent)
}

struct FContextHandler
{
	TWeakObjectPtr<ATileVisualizationActor> owner;
	
	// template <class Tx>
	// void handle(FSelectingUnitContext previouss, Tx nextt)
	// {
	// }
	//
	// template <class Tx>
	// void handle(Tx previouss, FSelectingUnitContext nextt)
	// {
	// }

	void handle(FSelectingUnitContext previouss, FSelectingUnitContext nextt)
	{
		if (!owner.IsValid() || owner->TileVisualizationComponent == nullptr)
		{
			return;
		}
		
		owner->TileVisualizationComponent->Clear();
		owner->TileVisualizationComponent->ShowTiles(nextt.interactionTiles.FindChecked(move), FLinearColor::Blue);
		owner->TileVisualizationComponent->ShowTiles(
			nextt.interactionTiles.FindChecked(attack)
			.Difference(nextt.interactionTiles.FindChecked(move)),
			FLinearColor::Red);
	}

	template <class Tx, class Ty>
	void handle(Tx previouss, Ty nextt)
	{
	}

	template <class Tx, class Ty>
	void operator()(Tx previouss, Ty nextt)
	{
		handle(previouss, nextt);
	}
};

// Called when the game starts or when spawned
void ATileVisualizationActor::BeginPlay()
{
	Super::BeginPlay();
	contextCursor = UseState(interactionContextLens).make();
	contextCursor.bind(TPreviousHookFunctor<TInteractionContext>(contextCursor.get(), [&](const auto& previous, const auto& next)
	{
		Visit(FContextHandler{this}, Forward<decltype(previous)>(previous), Forward<decltype(next)>(next));
		return next;
	}));
}

// Called every frame
void ATileVisualizationActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
