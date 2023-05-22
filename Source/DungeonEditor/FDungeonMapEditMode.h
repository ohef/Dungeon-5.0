#pragma once
#include "ADungeonWorld.h"
#include "DungeonUnitActor.h"
#include "EdMode.h"

class FDungeonMapEditorToolkit;

class DUNGEONEDITOR_API FDungeonMapEditMode : public FEdMode
{
public:
	static inline FEditorModeID EM_DungeonMap = TEXT("EM_DungeonMap");
	TWeakObjectPtr<ADungeonUnitActor> PreviewActor;
	TWeakObjectPtr<ADungeonWorld> DungeonWorld;
	TSharedPtr<FDungeonMapEditorToolkit> EditorModeToolkit;

	virtual void Initialize() override;

	virtual void Enter() override;

	virtual void Exit() override;

    virtual bool UsesToolkits() const override
    {
        return true;
    }

	virtual bool HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy,
	                         const FViewportClick& Click) override;
	virtual bool MouseMove(FEditorViewportClient* ViewportClient, FViewport* Viewport, int32 x, int32 y) override;

	virtual bool CapturedMouseMove(FEditorViewportClient* InViewportClient, FViewport* InViewport, int32 InMouseX,
	                               int32 InMouseY) override;
};
