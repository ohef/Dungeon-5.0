// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Dungeon.h"
#include "Misc/NotifyHook.h"
#include "Dungeon/Logic/unit.h"
#include "lager/store.hpp"

#include "DungeonMapEditor.generated.h"


UCLASS()
class UEditorModel : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	FDungeonLogicUnit DungeonLogicUnit;
};

using TDungeonEditorStore = lager::store<TStoreAction, FDungeonWorldState>;

class SDungeonEditor : public SCompoundWidget, public FNotifyHook
{
	SLATE_BEGIN_ARGS(SDungeonEditor)
		{
		}

		// High Level DragAndDrop
		/**
		 * Handle this event to determine whether a drag and drop operation can be executed on top of the target row widget.
		 * Most commonly, this is used for previewing re-ordering and re-organization operations in lists or trees.
		 * e.g. A user is dragging one item into a different spot in the list or tree.
		 *      This delegate will be called to figure out if we should give visual feedback on whether an item will 
		 *      successfully drop into the list.
		 */
		// SLATE_EVENT( FOnCanAcceptDrop, OnCanAcceptDrop )

		/**
		 * Perform a drop operation onto the target row widget
		 * Most commonly used for executing a re-ordering and re-organization operations in lists or trees.
		 * e.g. A user was dragging one item into a different spot in the list; they just dropped it.
		 *      This is our chance to handle the drop by reordering items and calling for a list refresh.
		 */
		// SLATE_EVENT( FOnAcceptDrop,    OnAcceptDrop )

		// Low level DragAndDrop
		// SLATE_EVENT( FOnDragDetected,      OnDragDetected )
		// SLATE_EVENT( FOnTableRowDragEnter, OnDragEnter )
		// SLATE_EVENT( FOnTableRowDragLeave, OnDragLeave )
		SLATE_EVENT(FOnTableRowDrop, OnDrop)
	SLATE_END_ARGS()

	virtual void NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent,
	                              class FEditPropertyChain* PropertyThatChanged) override;
	virtual FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
	virtual FReply OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;

	void Construct(const FArguments& Args);

	FIntPoint contextPosition;

	TUniquePtr<TDungeonEditorStore> editorStore;

	TStrongObjectPtr<UEditorModel> ContextModel;

	TSharedPtr<IDetailsView> CSVtoSVGArgumentsDetailsView = nullptr;
	TSharedPtr<IStructureDetailsView> StructPropEditor;
	TSharedPtr<TStructOnScope<FDungeonLogicUnit>> StructOnScope;
};
