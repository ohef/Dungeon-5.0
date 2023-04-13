#include "FDungeonMapEditMode.h"

#include "ADungeonWorld.h"
#include "AssetToolsModule.h"
#include "DungeonConstants.h"
#include "DungeonUnitActor.h"
#include "EditorModeManager.h"
#include "EditorUtilitySubsystem.h"
#include "Slate/Public/Slate.h"
#include "EngineUtils.h"
#include "IPropertyTable.h"
#include "ISinglePropertyView.h"
#include "IStructureDetailsView.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/AssertionMacros.h"
#include "Toolkits/ToolkitManager.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class FHelloWorldToolkit : public FModeToolkit
{
public:
	FHelloWorldToolkit()
	{
	}

	TSharedPtr<IPropertyTable> PropertyTable;
	TSharedPtr<IStructureDetailsView> PropertyEditor;

	void Init(const TSharedPtr<IToolkitHost>& InitToolkitHost, ADungeonWorld* World)
	{
		
		FPropertyEditorModule& PropertyEditorModule = FModuleManager::Get()
			.GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

		// FSinglePropertyParams Params;
		// TSharedPtr<ISinglePropertyView> PropertyView
		// = PropertyEditorModule.CreateSingleProperty( World, "currentWorldState", Params);

		auto EditThisShit = MakeShared<TStructOnScope<FDungeonWorldState>>();
		EditThisShit->InitializeAs<FDungeonWorldState>();
		FDetailsViewArgs Params2;
		FStructureDetailsViewArgs ViewArgs;
		PropertyEditor = PropertyEditorModule.CreateStructureDetailView(Params2, ViewArgs, EditThisShit);
		PropertyEditor->GetOnFinishedChangingPropertiesDelegate().AddLambda([EditThisShit,World](const FPropertyChangedEvent& event)
		{
			auto Property = CastField<FIntProperty>(event.Property);
			if (Property->GetFName() == GET_MEMBER_NAME_CHECKED(FTurnState, teamId))
			{
				World->WorldState->dispatch(CreateDungeonAction(FChangeTeam{
					.newTeamID = Property->GetPropertyValue(&EditThisShit->Get()->TurnState)
				}));
			}
			UE_LOG(LogTemp, Display, TEXT( "This is it %s" ), *event.Property->GetFName().ToString());
		});

		PropertyTable = PropertyEditorModule.CreatePropertyTable();
		PropertyTable->AddRow(World);
		
		DetailView = PropertyEditorModule.CreateDetailView(Params2);
		DetailView->SetObject(World);
		
		SAssignNew(HelloWorldButton, SVerticalBox)
		// + SVerticalBox::Slot()[
		// 	SNew(SEditableTextBox).Text(FText::FromString("Hey this is some shit dude you think you can"))
		// ]
		// + SVerticalBox::Slot()[
		// 	DetailView.ToSharedRef()
		// ]
		// + SVerticalBox::Slot()[
		// 	PropertyEditorModule.CreatePropertyTableWidget(PropertyTable.ToSharedRef())
		// ]
		+ SVerticalBox::Slot()[
			PropertyEditor->GetWidget().ToSharedRef()
		]
		;

		FModeToolkit::Init(InitToolkitHost);
	}

	virtual FName GetToolkitFName() const override
	{
		return FName("HelloWorldToolkit");
	}

	virtual TSharedPtr<SWidget> GetInlineContent() const override
	{
		return HelloWorldButton;
	}


	virtual FText GetBaseToolkitName() const override;
	virtual FDungeonMapEditMode* GetEditorMode() const override;

private:
	FReply OnHelloWorldButtonClicked()
	{
		UE_LOG(LogTemp, Display, TEXT("Hello World!"));
		return FReply::Handled();
	}

	TSharedPtr<IDetailsView> DetailView;
	TSharedPtr<SVerticalBox> HelloWorldButton;
};

FText FHelloWorldToolkit::GetBaseToolkitName() const
{
	return FText::FromString("HelloWorldToolkit");
}

FDungeonMapEditMode* FHelloWorldToolkit::GetEditorMode() const
{
	return (FDungeonMapEditMode*)GLevelEditorModeTools().GetActiveMode(FDungeonMapEditMode::EM_DungeonMap);
}

void FDungeonMapEditMode::Initialize()
{
	FEdMode::Initialize();
	
	// // Load the asset using the editor's object finder helper.
	// static ConstructorHelpers::FObjectFinder<ADungeonUnitActor> MyClassFinder(TEXT("/Game/Unit/Unit3.Unit3"));
	//
	//    // Get a reference to the AssetTools module.
	//    FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	//
	//    // Get a reference to the AssetRegistry module.
	//    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	//
	//    // Create a filter for the assets you want to find.
	//    FARFilter AssetFilter;
	//    AssetFilter.ObjectPaths.Add(TEXT("/Game/Unit/Unit3.Unit3"));
	//    AssetFilter.bRecursivePaths = true; // Search in all directories.
	//
	//    // Search for the assets that match the filter.
	//    TArray<FAssetData> AssetDataArray;
	//    AssetRegistryModule.Get().GetAssets(AssetFilter, AssetDataArray);

	// // Select the first texture asset found.
	// if (AssetDataArray.Num() > 0)
	// {
	//     FAssetData& AssetData = AssetDataArray[0];
	//     TArray<FAssetData> SelectedAssets = { AssetData };
	//
	//     // Use the AssetTools module to get a reference to the selected texture asset.
	//     TArray<UObject*> OutObjects;
	//     AssetToolsModule.Get().GetAssetRegistry().GetObjects(SelectedAssets, OutObjects);
	//
	//     if (OutObjects.Num() > 0)
	//     {
	//         UTexture2D* Texture = Cast<UTexture2D>(OutObjects[0]);
	//         if (Texture != nullptr)
	//         {
	//             // Do something with the texture asset.
	//             ...
	//         }
	//     }
	// }
}

void FDungeonMapEditMode::Enter()
{
	TSubclassOf<ADungeonWorld> filterClass = ADungeonWorld::StaticClass();
	DungeonWorld = FindAllActorsOfType(GetWorld(), filterClass);
	DungeonWorld->currentWorldState = {};
	DungeonWorld->currentWorldState.Map.Height = 15;
	DungeonWorld->currentWorldState.Map.Width = 15;
	DungeonWorld->currentWorldState.InteractionContext.Set<FSelectingUnitContext>({});
	DungeonWorld->currentWorldState.TurnState.teamId = 1;
	DungeonWorld->ResetStore();

	auto HelloWorldToolkit = new FHelloWorldToolkit;
	Toolkit = MakeShareable(HelloWorldToolkit);
	HelloWorldToolkit->Init(Owner->GetToolkitHost(), DungeonWorld.Get());

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

	auto wew = Cast<UBlueprint>(
		AssetRegistryModule.Get().GetAssetByObjectPath(TEXT("/Game/Unit/Unit3.Unit3")).GetAsset());

	// auto hey = GetWorld()->SpawnActor<AActor>(wew, FTransform(FVector(100, 100, 100)));
	// auto test = LoadClass<AActor>(nullptr,TEXT("/Game/Unit/Unit3.Unit3"));

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	SpawnParams.bTemporaryEditorActor = true;
	SpawnParams.bHideFromSceneOutliner = true;

	PreviewActor = GetWorld()->SpawnActor<ADungeonUnitActor>(wew->GeneratedClass, SpawnParams);
	// // Search for the assets that match the filter.
	// TArray<FAssetData> AssetDataArray;
	// AssetRegistryModule.Get().GetAssets(AssetFilter, AssetDataArray);
	//
	// // Select the first texture asset found.
	// if (AssetDataArray.Num() > 0)
	// {
	//     FAssetData& AssetData = AssetDataArray[0];
	//     TArray<FAssetData> SelectedAssets = { AssetData };
	//
	//     // Use the AssetTools module to get a reference to the selected texture asset.
	//     TArray<UObject*> OutObjects;
	//     AssetToolsModule.Get().GetAssetRegistry().GetObjects(SelectedAssets, OutObjects);
	//
	//     if (OutObjects.Num() > 0)
	//     {
	//         UTexture2D* Texture = Cast<UTexture2D>(OutObjects[0]);
	//         if (Texture != nullptr)
	//         {
	//             // Do something with the texture asset.
	//             ...
	//         }
	//     }
	// }

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
	auto unit = FDungeonLogicUnit();
	unit.Id = unitId++;
	unit.damage = 10;
	unit.attackRange = 1;
	unit.Movement = 3;
	unit.Name = "someName";
	unit.HitPoints = 20;
	unit.HitPointsTotal = 20;
	unit.teamId = DungeonWorld->WorldState->zoom(
		SimpleCastTo<FDungeonWorldState>
		| lager::lenses::attr(&FDungeonWorldState::TurnState)
		| lager::lenses::attr(&FTurnState::teamId)).make().get();

	DungeonWorld->WorldState->dispatch(CreateDungeonAction(FSpawnUnit{
		.Position = CursorPosition,
		.Unit = unit,
		.PrefabClass = "/Game/Unit/Unit3.Unit3"
	}));

	// auto supNigga = InViewportClient->GetWorld()->GetCurrentLevel();
	// for (auto Element : supNigga->Actors)
	// {
	// }
	//
	// FEdMode::HandleClick(InViewportClient, HitProxy, Click);

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
		auto PrimComponent = const_cast<UPrimitiveComponent*>(hitActor->PrimComponent);
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
