#pragma once
#include "Dungeon.h"
#include "EngineUtils.h"
#include "lager/store.hpp"

#include "ADungeonWorld.generated.h"

UCLASS()
class DUNGEON_API ADungeonWorld : public AActor
{
	GENERATED_BODY()
public:
	ADungeonWorld();

	TUniquePtr<FDungeonStore> WorldState;

	lager::cursor<FHistoryModel> WorldCursor;
	
	UPROPERTY(VisibleAnywhere)
	FDungeonWorldState currentWorldState;

	void ResetStore();

	virtual void PostLoad() override;

	virtual void PostInitializeComponents() override;

	virtual void BeginPlay() override;

public:
	UFUNCTION(BlueprintCallable)
	void HandleChangeToState();
};

template<typename T>
inline TWeakObjectPtr<T> FindAllActorsOfType(UWorld* World, TSubclassOf<T> ActorClass)
{
	for (TActorIterator<T> Iter(World, ActorClass); Iter; ++Iter)
	{
		return *Iter;
		// // Do something with the actor, for example:
		// UE_LOG(LogTemp, Warning, TEXT("Found actor of type %s"), *ActorClass->GetName());
	}
	return nullptr;
}