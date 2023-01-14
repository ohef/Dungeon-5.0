// Fill out your copyright notice in the Description page of Project Settings.


#include "DungeonMapEditor.h"

#include "DataTableEditorUtils.h"
#include "IStructureDetailsView.h"
#include "DungeonGameModeBase.h"
#include "EditorAssetLibrary.h"
#include "JsonObjectConverter.h"
#include "Dungeon/DungeonReducer.hpp"
#include "lager/event_loop/manual.hpp"
#include "Styling/UMGCoreStyle.h"
#include "Widgets/Layout/SGridPanel.h"
#include "SDataTableListViewRow.h"

void SDungeonEditor::NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent,
                                      FEditPropertyChain* PropertyThatChanged)
{
	FNotifyHook::NotifyPostChange(PropertyChangedEvent, PropertyThatChanged);
}

DECLARE_DELEGATE_TwoParams(FTileDragEnter, const FGeometry&, const FDragDropEvent&)

class SDungeonEditorTile : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SDungeonEditorTile)
		{
		}

		SLATE_EVENT(FTileDragEnter, TileDragEnter)
	SLATE_END_ARGS()

	void Construct(const FArguments& Args)
	{
		this->ChildSlot[SNew(SButton)];
	}

	virtual FReply OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override
	{
		return FReply::Unhandled();
	}
};


FReply SDungeonEditor::OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	TSharedPtr<FDataTableRowDragDropOp> DataTableRowDragDropOp = DragDropEvent.GetOperationAs<
		FDataTableRowDragDropOp>();
	auto JSONPayload = DataTableRowDragDropOp->Row.Pin()->GetRowDataPtr()->CellData[0]; // The actual payload
	FDungeonLogicUnit Unit;
	FJsonObjectConverter::JsonObjectStringToUStruct(JSONPayload.ToString(), &Unit, 0, 0);
	*UnitOnScope->Get() = Unit;

	return FReply::Unhandled();
}

FReply SDungeonEditor::OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	auto hey = DragDropEvent.GetEventPath();
	return FReply::Unhandled();
}

void SDungeonEditor::Construct(const FArguments& Args)
{
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::Get().GetModuleChecked<FPropertyEditorModule>(
		"PropertyEditor");

	editorStore = MakeUnique<TDungeonEditorStore>(lager::make_store<TStoreAction>(
		FDungeonWorldState{.Map = {.Width = 10, .Height = 10}},
		lager::with_manual_event_loop{},
		lager::with_reducer(WorldStateReducer)
	));

	// IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
	// AssetTools.asset

	auto wew = Cast<UMapDataHolder>(
		UEditorAssetLibrary::LoadAsset("MapDataHolder'/Game/Blueprints/Widgets/NewDataAsset.NewDataAsset'"));
	wew->map.Height = 10;
	wew->map.Width = 10;
	UEditorAssetLibrary::SaveAsset("MapDataHolder'/Game/Blueprints/Widgets/NewDataAsset.NewDataAsset'");

	const FMargin PaddingAmount(2.0f);

	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsViewArgs.bAllowSearch = false;
	DetailsViewArgs.NotifyHook = this;
	DetailsViewArgs.bShowOptions = true;
	DetailsViewArgs.bShowModifiedPropertiesOption = true;

	UnitOnScope = MakeShared<TStructOnScope<FDungeonLogicUnit>>(MakeStructOnScope<FDungeonLogicUnit>());
	DataOnScope = MakeShared<TStructOnScope<FTileData>>(MakeStructOnScope<FTileData>());

	// FPropertyRowGeneratorArgs args;
	// TSharedRef<IPropertyRowGenerator> anotherOne = PropertyEditorModule.CreatePropertyRowGenerator(args);
	// anotherOne->SetStructure(UnitOnScope);

	// ClassDetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);

	StructPropEditor = PropertyEditorModule.CreateStructureDetailView(
		DetailsViewArgs, FStructureDetailsViewArgs{},
		DataOnScope);

	auto width = editorStore->get().Map.Width;
	auto height = editorStore->get().Map.Height;
	auto GridPanel = SNew(SGridPanel);

	for (int x = 0; x < width; x++)
	{
		for (int y = 0; y < height; y++)
		{
			GridPanel->AddSlot(x, y, SGridPanel::Layer{1})
			         .Padding(PaddingAmount)
			[
				SNew(SButton)
				.ButtonStyle(FUMGCoreStyle::Get(), "Button")
				.Text_Lambda([pt = FIntPoint{x, y},this]
				             {
					             const auto& possibleUnit = lager::view(getUnitAtPointLens(pt), editorStore->get());
					             return possibleUnit.IsSet() ? FText::AsNumber(possibleUnit.GetValue()) : FText();
				             })
				.OnClicked_Lambda([x,y,this]
				             {
					             contextPosition = {x, y};
					             auto possibleUnit = lager::view(getUnitAtPointLens(contextPosition),
					                                             editorStore->get());

					             *UnitOnScope->Get() = lager::view(unitDataLens(possibleUnit.Get(-1)),
					                                               editorStore->get()).Get({});

					             return FReply::Handled();
				             })
			];
		}
	}

	this->ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		[GridPanel]
		+ SVerticalBox::Slot()
		.Padding(PaddingAmount)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().Padding(PaddingAmount)
			[StructPropEditor->GetWidget()->AsShared()]
			+ SVerticalBox::Slot()
			[SNew(SButton)
				.Text(FText::FromString("Write to cell"))
				.OnClicked_Lambda([this]
			              {
				              editorStore->dispatch(CreateDungeonAction(FSpawnUnit{
					              .position = contextPosition, .unit = *UnitOnScope->Get()
				              }));
				              return FReply::Handled();
			              })]
		]
	];

	// editorStore->dispatch(CreateDungeonAction(FMoveAction(2, {1,2})));
	// editorStore->dispatch(CreateDungeonAction(FCombatAction(2)));
}
