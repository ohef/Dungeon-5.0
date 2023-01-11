// // Fill out your copyright notice in the Description page of Project Settings.
//
//
// #include "DungeonEditor.h"
//
// #include "lager/event_loop/manual.hpp"
// #include "DungeonReducer.hpp"
// #include "IStructureDetailsView.h"
// #include "Styling/UMGCoreStyle.h"
// #include "Widgets/Layout/SGridPanel.h"
//
// void SDungeonEditor::NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent,
//                                       FEditPropertyChain* PropertyThatChanged)
// {
// 	FNotifyHook::NotifyPostChange(PropertyChangedEvent, PropertyThatChanged);
// }
//
// DECLARE_DELEGATE_TwoParams(FTileDragEnter, const FGeometry&, const FDragDropEvent&)
//
// class DUNGEON_API SDungeonEditorTile : public SCompoundWidget
// {
// 	SLATE_BEGIN_ARGS(SDungeonEditorTile)
// 	{
// 	}
// 	
// 	SLATE_EVENT(FTileDragEnter, TileDragEnter)
// 	SLATE_END_ARGS()
//
// 	void Construct(const FArguments& Args)
// 	{
// 		this->ChildSlot[SNew(SButton)];
// 	}
//
// 	virtual FReply OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override
// 	{
// 		return FReply::Unhandled();
// 	}
// };
//
// FReply SDungeonEditor::OnDrop( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent )
// {
// 	// auto wowGreatFuckYu = DragDropEvent.GetOperationAs<FDataTableRowDragDropOp>();
// 	// FDataTableEditorRowListViewDataPtr anotherTest = wowGreatFuckYu->Row.Pin()->GetRowDataPtr();
// 	// auto hey = DragDropEvent.GetEventPath();
// 	return FReply::Unhandled();
// }
//
// FReply SDungeonEditor::OnDragOver( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent )
// {
// 	auto hey = DragDropEvent.GetEventPath();
// 	return FReply::Unhandled();
// }
//
// void SDungeonEditor::Construct(const FArguments& Args)
// {
// 	FPropertyEditorModule& PropertyEditorModule = FModuleManager::Get().GetModuleChecked<FPropertyEditorModule>(
// 		"PropertyEditor");
//
// 	editorStore = MakeUnique<TDungeonEditorStore>(lager::make_store<TStoreAction>(
// 		FDungeonWorldState{.Map = {.Width = 10, .Height = 10}},
// 		lager::with_manual_event_loop{},
// 		lager::with_reducer(WorldStateReducer)
// 	));
//
// 	const FMargin PaddingAmount(2.0f);
//
// 	FDetailsViewArgs DetailsViewArgs;
// 	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
// 	DetailsViewArgs.bAllowSearch = false;
// 	DetailsViewArgs.NotifyHook = this;
// 	DetailsViewArgs.bShowOptions = true;
// 	DetailsViewArgs.bShowModifiedPropertiesOption = true;
//
// 	StructOnScope = MakeShared<TStructOnScope<FDungeonLogicUnit>>(MakeStructOnScope<FDungeonLogicUnit>());
// 	StructPropEditor = PropertyEditorModule.CreateStructureDetailView(
// 		DetailsViewArgs, FStructureDetailsViewArgs{},
// 		StructOnScope);
// 	
// 	// ContextModel = TStrongObjectPtr(NewObject<UEditorModel>());
// 	// CSVtoSVGArgumentsDetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
// 	// CSVtoSVGArgumentsDetailsView->SetObject(ContextModel.Get());
//
// 	auto width = editorStore->get().Map.Width;
// 	auto height = editorStore->get().Map.Height;
// 	auto GridPanel = SNew(SGridPanel);
//
// 	for (int x = 0; x < width; x++)
// 	{
// 		for (int y = 0; y < height; y++)
// 		{
// 			GridPanel->AddSlot(x, y, SGridPanel::Layer{1})
// 			         .Padding(PaddingAmount)
// 			[
// 				SNew(SButton)
// 				.ButtonStyle(FUMGCoreStyle::Get(), "Button")
// 				.OnClicked_Lambda([x,y,this]
// 				{
// 					contextPosition = {x,y};
// 					auto possibleUnit = lager::view(getUnitAtPointLens(contextPosition), editorStore->get());
// 					
// 					*StructOnScope->Get() = lager::view(unitDataLens(possibleUnit.Get(-1)), editorStore->get()).Get({});
// 					
// 					return FReply::Handled();
// 				})
// 			];
// 		}
// 	}
//
// 	this->ChildSlot
// 	[
// 		SNew(SVerticalBox)
// 		+ SVerticalBox::Slot()
// 		[GridPanel]
// 		+ SVerticalBox::Slot()
// 		.Padding(PaddingAmount)
// 		[
// 			SNew(SVerticalBox)
// 			+ SVerticalBox::Slot().Padding(PaddingAmount)
// 			[StructPropEditor->GetWidget()->AsShared()]
// 			+ SVerticalBox::Slot()
// 			[SNew(SButton).Text(FText::FromString("Write to cell")).OnClicked_Lambda([this]
// 			{
// 				editorStore->dispatch(CreateDungeonAction(FSpawnUnit{.position = contextPosition, .unit = *StructOnScope->Get()}));
// 				return FReply::Handled();
// 			})]
// 		]
// 	];
//
// 	// editorStore->dispatch(CreateDungeonAction(FMoveAction(2, {1,2})));
// 	// editorStore->dispatch(CreateDungeonAction(FCombatAction(2)));
// }
