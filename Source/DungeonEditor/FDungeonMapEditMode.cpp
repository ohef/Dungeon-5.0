#include "FDungeonMapEditMode.h"

#include "ADungeonWorld.h"
#include "AssetToolsModule.h"
#include "DataTableEditorModule.h"
#include "DataTableEditorUtils.h"
#include "DungeonConstants.h"
#include "DungeonUnitActor.h"
#include "EditorModeManager.h"
#include "EditorUtilitySubsystem.h"
#include "Slate/Public/Slate.h"
#include "EngineUtils.h"
#include "IDataTableEditor.h"
#include "IPropertyTable.h"
#include "ISinglePropertyView.h"
#include "DungeonWorldEditorState.h"
#include "IDetailTreeNode.h"
#include "IStructureDetailsView.h"
#include "Algo/Accumulate.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Lenses/model.hpp"
#include "Misc/AssertionMacros.h"
#include "Toolkits/ToolkitManager.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

/**
 * 
 */
class DUNGEONEDITOR_API SEditorRow : public SMultiColumnTableRow<TSharedRef<FDungeonLogicUnitRow>>
{
public:
	SLATE_BEGIN_ARGS(SEditorRow)
		{
		}

	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	virtual void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTableView)
	{
		SMultiColumnTableRow<TSharedRef<FDungeonLogicUnitRow>>::Construct(
			SMultiColumnTableRow<TSharedRef<FDungeonLogicUnitRow>>::FArguments(), OwnerTableView);
	}

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& InColumnName) override
	{
		UScriptStruct* ScriptStruct = FDungeonLogicUnitRow::StaticStruct();
		auto daProp = ScriptStruct->FindPropertyByName(InColumnName);
		if (InColumnName == GET_MEMBER_NAME_CHECKED(FDungeonLogicUnitRow, unitData))
		{
			// UE_DEBUG_BREAK();
		}
		else if (InColumnName == GET_MEMBER_NAME_CHECKED(FDungeonLogicUnitRow, UnrealActorAsset))
		{
			// UE_DEBUG_BREAK();
		}

		return SNew(STextBlock).Text(FText::FromName(InColumnName));
	}
};

class FDungeonMapEditorToolkit : public FModeToolkit
{
public:
	FDungeonMapEditorToolkit()
	{
	}

	TArray<TSharedRef<FDungeonLogicUnitRow>> UnitEntries;
	TSharedPtr<IStructureDetailsView> PropertyEditor;
	TSharedPtr<TStructOnScope<FDungeonWorldEditorState>> BlankStateEditing;
	TSharedPtr<SListView<TSharedRef<FDungeonLogicUnitRow>>> ListView;
	TSharedPtr<SHeaderRow> ThisHeader;
	TWeakObjectPtr<ADungeonWorld> WorldPtr;

	void Init(const TSharedPtr<IToolkitHost>& InitToolkitHost, ADungeonWorld* World)
	{
		WorldPtr = World;
		FPropertyEditorModule& PropertyEditorModule = FModuleManager::Get()
			.GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

		BlankStateEditing = MakeShared<TStructOnScope<FDungeonWorldEditorState>>();
		BlankStateEditing->InitializeAs<FDungeonWorldEditorState>();

		FDetailsViewArgs Params2;
		FStructureDetailsViewArgs ViewArgs;
		PropertyEditor = PropertyEditorModule.CreateStructureDetailView(Params2, ViewArgs, BlankStateEditing);
		PropertyEditor->GetOnFinishedChangingPropertiesDelegate().AddLambda(
			[this,World](const FPropertyChangedEvent& event)
			{
				if (event.Property->GetFName() == GET_MEMBER_NAME_CHECKED(FTurnState, teamId))
				{
					auto Property = CastField<FIntProperty>(event.Property);
					World->WorldState->dispatch(CreateDungeonAction(FChangeTeam{
						.newTeamID = Property->GetPropertyValue(&BlankStateEditing->Get()->WorldState.TurnState)
					}));
				}
				else if (event.Property->GetFName() == GET_MEMBER_NAME_CHECKED(FDungeonWorldEditorState, Table))
				{
					auto Property = CastField<FObjectProperty>(event.Property);
					auto anotherOne = BlankStateEditing->Get();
					auto rando = Property->GetObjectPropertyValue(anotherOne);
					auto wew = Cast<UDataTable>(rando);

					TArray<FDungeonLogicUnitRow*> out;
					anotherOne->Table->GetAllRows("", out);

					UnitEntries.Empty();
					Algo::Transform(out, UnitEntries, [](auto x) { return MakeShared<FDungeonLogicUnitRow>(*x); });
					ListView->RebuildList();
					// UE_DEBUG_BREAK();
				}

				// UE_LOG(LogTemp, Display, TEXT( "This is it %s" ), *event.Property->GetFName().ToString());
			});

		UScriptStruct* ScriptStruct = FDungeonLogicUnitRow::StaticStruct();
		FField* field = ScriptStruct->ChildProperties;
		SAssignNew(ThisHeader, SHeaderRow);

		while (field != nullptr)
		{
			ThisHeader->AddColumn(
				SHeaderRow::Column(field->GetFName())
				.DefaultLabel(FText::FromString(field->GetName())));
			field = field->Next;
		}

		SAssignNew(SVerticalBoxEditor, SVerticalBox)
			+ SVerticalBox::Slot()[
				PropertyEditor->GetWidget().ToSharedRef()
			]
			+ SVerticalBox::Slot()[
				SAssignNew(ListView, SListView<TSharedRef<FDungeonLogicUnitRow>>)
				.OnSelectionChanged(this, &FDungeonMapEditorToolkit::OnSelectionChanged)
				.HeaderRow(ThisHeader)
				.OnGenerateRow(this, &FDungeonMapEditorToolkit::OnGenerateRowForList)
				.ListItemsSource(&UnitEntries)
			];

		FModeToolkit::Init(InitToolkitHost);
	}

	void OnSelectionChanged(SListView<TSharedRef<FDungeonLogicUnitRow>>::NullableItemType item, ESelectInfo::Type type)
	{
		if (BlankStateEditing.IsValid() && BlankStateEditing->IsValid() && item.IsValid())
		{
			BlankStateEditing->Get()->CurrentUnit = *item;
		}
	}

	TSharedRef<class ITableRow> OnGenerateRowForList(TSharedRef<FDungeonLogicUnitRow> arg,
	                                                 const TSharedRef<class STableViewBase>& table)
	{
		return SNew(SEditorRow, table);
	}

	virtual FName GetToolkitFName() const override
	{
		return FName("HelloWorldToolkit");
	}

	virtual TSharedPtr<SWidget> GetInlineContent() const override
	{
		return SVerticalBoxEditor;
	}

	virtual FText GetBaseToolkitName() const override;
	virtual FDungeonMapEditMode* GetEditorMode() const override;

private:
	
	TSharedPtr<SVerticalBox> SVerticalBoxEditor;
};

FText FDungeonMapEditorToolkit::GetBaseToolkitName() const
{
	return FText::FromString("HelloWorldToolkit");
}

FDungeonMapEditMode* FDungeonMapEditorToolkit::GetEditorMode() const
{
	return (FDungeonMapEditMode*)GLevelEditorModeTools().GetActiveMode(FDungeonMapEditMode::EM_DungeonMap);
}

void FDungeonMapEditMode::Initialize()
{
	FEdMode::Initialize();
}

void FDungeonMapEditMode::Enter()
{
	TSubclassOf<ADungeonWorld> filterClass = ADungeonWorld::StaticClass();
	DungeonWorld = FindAllActorsOfType(GetWorld(), filterClass);

	FDataTableEditorModule& DataTableEditorModule = FModuleManager::LoadModuleChecked<FDataTableEditorModule>(
		"DataTableEditor");

	EditorModeToolkit = MakeShareable(new FDungeonMapEditorToolkit);
	Toolkit = EditorModeToolkit;
	EditorModeToolkit->Init(Owner->GetToolkitHost(), DungeonWorld.Get());

	// UDataTable* UnitTable = NewObject<UDataTable>();
	// UnitTable->RowStruct = FDungeonLogicUnit::StaticStruct();
	// auto TheEditorThing = DataTableEditorModule
	// 	.CreateDataTableEditor(EToolkitMode::Type::Standalone, Owner->GetToolkitHost(), UnitTable);
	// TheEditorThing->GetEditingObject();

	for (TActorIterator<ADungeonUnitActor> Iter(GetWorld(), ADungeonUnitActor::StaticClass()); Iter; ++Iter)
	{
		Iter->Destroy();
	}

	// Get a reference to the AssetRegistry module.
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<
		FAssetRegistryModule>("AssetRegistry");

	// Create a filter for the assets you want to fin.
	FARFilter AssetFilter;
	AssetFilter.ObjectPaths.Add(TEXT("/Game/Unit/Unit3.Unit3"));
	AssetFilter.bRecursivePaths = true; // Search in all directories.

	auto UnitActor = Cast<UBlueprint>(
		AssetRegistryModule.Get().GetAssetByObjectPath(TEXT("/Game/Unit/Unit3.Unit3")).GetAsset());

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	SpawnParams.bTemporaryEditorActor = true;
	SpawnParams.bHideFromSceneOutliner = true;

	PreviewActor = GetWorld()->SpawnActor<ADungeonUnitActor>(UnitActor->GeneratedClass, SpawnParams);

	FEdMode::Enter();
}

void FDungeonMapEditMode::Exit()
{
	PreviewActor->Destroy();
	DungeonWorld->currentWorldState.CursorPosition = {0, 0};
	FToolkitManager::Get().CloseToolkit(Toolkit.ToSharedRef());
	Toolkit.Reset();

	FEdMode::Exit();
}

static inline int unitId = 1;

bool FDungeonMapEditMode::HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy,
                                      const FViewportClick& Click)
{
	auto CursorPosition = DungeonWorld->WorldState->zoom(
		SimpleCastTo<FDungeonWorldState> |
		lager::lenses::attr(&FDungeonWorldState::CursorPosition)).make().get();
	
	auto unit = EditorModeToolkit->BlankStateEditing->Get()->CurrentUnit;
	unit.unitData.Id = ++unitId;
	
	DungeonWorld->WorldState->dispatch(CreateDungeonAction(FSpawnUnit{
		.Position = CursorPosition,
		.Unit = unit.unitData,
		.PrefabClass = unit.UnrealActorAsset.GetAssetPathString()
	}));

	auto justAddedThis = DungeonWorld->WorldState->zoom(
		SimpleCastTo<FDungeonWorldState> | unitIdToActor(unit.unitData.Id)).make().get();
	TDungeonActionDispatched wew;
	justAddedThis->CreateDynamicMaterials();
	justAddedThis->HookIntoStore(*DungeonWorld->WorldState, wew);

	// auto didChange = EditorModeToolkit->BlankStateEditing->Get()->Table;
	
	return true;
}

bool FDungeonMapEditMode::MouseMove(FEditorViewportClient* ViewportClient, FViewport* Viewport, int32 x, int32 y)
{
	return FDungeonMapEditMode::CapturedMouseMove(ViewportClient, Viewport, x, y);
}

bool FDungeonMapEditMode::CapturedMouseMove(FEditorViewportClient* InViewportClient, FViewport* InViewport,
                                            int32 InMouseX, int32 InMouseY)
{
	// Ignore actor manipulation if we're using a tool
	// if ( !MouseDeltaTracker->UsingDragTool() )
	// {
	// EInputEvent Event = InputState.GetInputEvent();
	FViewport* InputStateViewport = InViewport;
	// FKey Key = InputState.GetKey();

	const int32 HitX = InputStateViewport->GetMouseX();
	const int32 HitY = InputStateViewport->GetMouseY();

	// Calc the raw delta from the mouse to detect if there was any movement
	// FVector RawMouseDelta = MouseDeltaTracker->GetRawDelta();

	// Note: We are using raw mouse movement to double check distance moved in low performance situations.  In low performance situations its possible
	// that we would get a mouse down and a mouse up before the next tick where GEditor->MouseMovment has not been updated.
	// In that situation, legitimate drags are incorrectly considered clicks
	// bool bNoMouseMovment = RawMouseDelta.SizeSquared() < MOUSE_CLICK_DRAG_DELTA && GEditor->MouseMovement.SizeSquared() < MOUSE_CLICK_DRAG_DELTA;

	// If the mouse haven't moved too far, treat the button release as a click.
	// if( bNoMouseMovment && !MouseDeltaTracker->WasExternalMovement() )
	// {


	TRefCountPtr<HHitProxy> HitProxy = InputStateViewport->GetHitProxy(HitX, HitY);
	HActor* hitActor = static_cast<HActor*>(HitProxy.GetReference());
	if (hitActor != nullptr)
	{
		FSceneViewFamilyContext ViewFamily(
			FSceneViewFamily::ConstructionValues(InViewport, InViewportClient->GetScene(),
			                                     FEngineShowFlags(ESFIM_Game)));
		FSceneView* View = InViewportClient->CalcSceneView(&ViewFamily);
		FViewportCursorLocation vsl = FViewportCursorLocation(
			View, InViewportClient, InMouseX, InMouseY);

		struct FHitResult OutHit;

		if (hitActor == nullptr && hitActor->PrimComponent == nullptr)
		{
			return true;
		}

		if (!hitActor->PrimComponent)
		{
			return false;
		}
		
		auto PrimComponent = MakeWeakObjectPtr(const_cast<UPrimitiveComponent*>(hitActor->PrimComponent));
		if (PrimComponent == nullptr)
			return false;

		FCollisionQueryParams Stuff{};
		bool bLineTraceComponent = PrimComponent->LineTraceComponent(OutHit, vsl.GetOrigin(), vsl.GetDirection() * 1e7,
		                                                             Stuff);

		const FIntPoint quantized = FIntPoint{
			(FMath::CeilToInt((OutHit.ImpactPoint.X - TILE_POINT_SCALE * .5) / TILE_POINT_SCALE)),
			(FMath::CeilToInt((OutHit.ImpactPoint.Y - TILE_POINT_SCALE * .5) / TILE_POINT_SCALE))
		};

		FVector toPoint = FVector(quantized) * TILE_POINT_SCALE + FVector{0, 0, 1.0};
		DungeonWorld->WorldState->dispatch(CreateDungeonAction(FCursorPositionUpdated{quantized}));

		PreviewActor->SetActorLocation(toPoint);
	}


	// When clicking, the cursor should always appear at the location of the click and not move out from undere the user
	// InputStateViewport->SetPreCaptureMousePosFromSlateCursor();
	// 	ProcessClick(View,HitProxy,Key,Event,HitX,HitY);
	// }
	// }

	return FEdMode::CapturedMouseMove(InViewportClient, InViewport, InMouseX, InMouseY);
}
