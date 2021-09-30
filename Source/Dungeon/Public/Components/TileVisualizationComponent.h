#include "CoreMinimal.h"
#include "DungeonConstants.h"
#include "Dungeon/DungeonGameModeBase.h"

#include "TileVisualizationComponent.generated.h"

UCLASS(meta = (BlueprintSpawnableComponent), Blueprintable)
class DUNGEON_API UTileVisualizationComponent : public UInstancedStaticMeshComponent
{
	GENERATED_BODY()

public:
	explicit UTileVisualizationComponent(const FObjectInitializer& ObjectInitializer)
		: UInstancedStaticMeshComponent(ObjectInitializer)
	{
	}

	void ShowTiles(const TArray<FIntPoint>& points)
	{
		for (auto point : points)
		{
			this->AddInstance(FTransform(FVector(point) * TILE_POINT_SCALE + FVector{0,0,1} ));
		}
	}

	void Clear()
	{
		this->ClearInstances();
	}
};