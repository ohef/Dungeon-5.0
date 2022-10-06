#pragma once

#include "CoreMinimal.h"
#include "DungeonConstants.h"

#include "TileVisualizationComponent.generated.h"

UCLASS(meta = (BlueprintSpawnableComponent), Blueprintable)
class DUNGEON_API UTileVisualizationComponent : public UInstancedStaticMeshComponent
{
  GENERATED_BODY()

public:
  explicit UTileVisualizationComponent(const FObjectInitializer& ObjectInitializer)
    : UInstancedStaticMeshComponent(ObjectInitializer)
  {
    this->NumCustomDataFloats = 3;
  }

  template<typename TContainer = TSet<FIntPoint>>
  void ShowTiles(const TContainer& points, const FLinearColor& color = FLinearColor::Black)
  {
    for (auto point : points)
    {
      int32 AddInstance = this->AddInstance(FTransform(FVector(point) * TILE_POINT_SCALE + FVector{0, 0, 1}));

      if (color == FColor::Black)
        continue;

      this->SetCustomData( AddInstance, {color.R,color.G,color.B});
    }
  }

  void Clear()
  {
    this->ClearInstances();
  }
};
