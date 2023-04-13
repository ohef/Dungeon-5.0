#include "ADungeonWorld.h"

#include "lager/setter.hpp"
#include "DungeonReducer.hpp"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "lager/event_loop/manual.hpp"

ADungeonWorld::ADungeonWorld()
{
}

void ADungeonWorld::ResetStore()
{
	WorldState = MakeUnique<FDungeonStore>(FDungeonStore(lager::make_store<TStoreAction>(
		FHistoryModel{currentWorldState},
		lager::with_manual_event_loop{},
		lager::with_deps(std::ref(*GetWorld())),
		WithUndoReducer(WorldStateReducer)
	)));

	WorldCursor = WorldState->setter([](FHistoryModel value)
	{
	});
	
	WorldState->bind([this](auto model)
	{
		this->currentWorldState = model;
	});
}

void ADungeonWorld::PostLoad()
{
	Super::PostLoad();
	ResetStore();
}

void ADungeonWorld::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}

void ADungeonWorld::BeginPlay()
{
	Super::BeginPlay();
}

void ADungeonWorld::HandleChangeToState()
{
	// WorldState = over(attr(&FDungeonWorldState::unitIdToActor), WorldState, [&](TMap<int, TWeakObjectPtr<ADungeonUnitActor>> idToActorMap)
	// {
	// 	TSet<int> LoadedUnits;
	// 	WorldState.Map.LoadedUnits.GetKeys(LoadedUnits);
	//
	// 	TSet<int> LoadedActorsToUnits;
	// 	idToActorMap.GetKeys(LoadedActorsToUnits);
	//
	// 	auto changedSet = LoadedUnits.Difference(LoadedActorsToUnits);
	//
	// 	for (auto LoadedUnit : changedSet)
	// 	{
	// 	}
	// });
}
