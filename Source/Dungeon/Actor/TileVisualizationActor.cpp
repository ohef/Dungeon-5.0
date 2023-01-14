// Fill out your copyright notice in the Description page of Project Settings.


#include "Actor/TileVisualizationActor.h"

#include "DungeonReducer.hpp"
#include "Lenses/model.hpp"
#include "Dungeon/DungeonGameModeBase.h"
#include "Logic/util.h"

#define FastSubobject(FieldName) \
FieldName = CreateDefaultSubobject<TRemovePointer<decltype(FieldName)>::Type>(#FieldName);

ATileVisualizationActor::ATileVisualizationActor(const FObjectInitializer& ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;

	FastSubobject(MovementVisualizationComponent)
	FastSubobject(TileVisualizationComponent)
}

// Called when the game starts or when spawned
void ATileVisualizationActor::BeginPlay()
{
	Super::BeginPlay();

	UseEvent().AddLambda(Dungeon::MatchEffect([&](const FBackAction& action)
	{
		contextCursor.nudge();
	}));

	contextCursor = UseState(interactionContextLens).make();
	contextCursor.bind(Dungeon::MatchEffect(
			[&](const FSelectingUnitAbilityTarget& context)
			{
				const auto& model = this->UseViewState();
				if ( model.WaitingForResolution.IsType<FMoveAction>() )
				{					
					this->TileVisualizationComponent->Clear();

					auto [MovementPoints,AttackPoints] = GetInteractionFields(
						model, model.WaitingForResolution.Get<FMoveAction>().InitiatorId);

					this->TileVisualizationComponent->ShowTiles(MovementPoints, FLinearColor::Blue);
					this->TileVisualizationComponent->ShowTiles(AttackPoints.Difference(MovementPoints),
					                                            FLinearColor::Red);
				}
			},
			[&](const FSelectingUnitContext& context)
			{
				if (this->TileVisualizationComponent == nullptr)
				{
					return;
				}

				this->TileVisualizationComponent->Clear();

				const auto& model = this->UseViewState();
				TOptional<FDungeonLogicUnit> unit = lager::view(getUnitUnderCursor, model);

				if (unit.IsSet())
				{
					auto [MovementPoints,AttackPoints] = GetInteractionFields(model, unit->Id);
					this->TileVisualizationComponent->ShowTiles(MovementPoints,
					                                            FLinearColor::Blue);
					this->TileVisualizationComponent->ShowTiles(AttackPoints.Difference(MovementPoints),
					                                            FLinearColor::Red);
				}
			},
			[&](const FUnitMenu& menu)
			{
				if (this->TileVisualizationComponent == nullptr)
				{
					return;
				}

				const auto& model = this->UseViewState();

				if (menu.focusedAbilityName == FName("Move"))
				{
					this->TileVisualizationComponent->Clear();

					auto [MovementPoints,AttackPoints] = GetInteractionFields(model, menu.unitId);

					this->TileVisualizationComponent->ShowTiles(MovementPoints, FLinearColor::Blue);
					this->TileVisualizationComponent->ShowTiles(AttackPoints.Difference(MovementPoints),
					                                            FLinearColor::Red);
				}

				if (menu.focusedAbilityName == FName("Attack"))
				{
					auto attackTiles = manhattanReachablePoints(
						model.Map.Width,
						model.Map.Height,
						lager::view(unitIdToData(menu.unitId), model).attackRange,
						lager::view(unitIdToPosition(menu.unitId), model));

					this->TileVisualizationComponent->Clear();
					this->TileVisualizationComponent->ShowTiles(attackTiles, FLinearColor::Red);
				}
			}
		)
	);
}

// Called every frame
void ATileVisualizationActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
