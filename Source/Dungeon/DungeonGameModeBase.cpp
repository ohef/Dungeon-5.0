// Copyright Epic Games, Inc. All Rights Reserved.

#include "DungeonGameModeBase.h"

#include "DungeonReducer.hpp"
#include "Logic/DungeonGameState.h"
#include "SingleSubmitHandler.h"
#include "Actor/DungeonPlayerController.h"
#include "Actor/TileVisualizationActor.h"
#include "DungeonUnitActor.h"
#include "Widget/DungeonMainWidget.h"
#include "JsonObjectConverter.h"
#include "Algo/Accumulate.h"
#include "Engine/DataTable.h"
#include "Engine/StaticMeshActor.h"
#include "GameFramework/GameStateBase.h"
#include "Interfaces/IMainFrameModule.h"
#include "Kismet/KismetSystemLibrary.h"
#include "lager/event_loop/manual.hpp"
#include "lager/store.hpp"
#include "lager/util.hpp"
#include "Lenses/model.hpp"
#include "Logic/SimpleTileGraph.h"
#include "Serialization/Csv/CsvParser.h"

template <typename T>
struct TIsOptional
{
	const static bool Value = false;
};

template <typename T>
struct TIsOptional<TOptional<T>>
{
	const static bool Value = true;
	using Type = T;
};

FReply UViewingModel::GenerateMoves()
{
	using namespace lager::lenses;
	using namespace lager;

	const FDungeonWorldState& initialState = currentModel;

	// state.TurnState.teamId = 1;
	auto aiTeamId = initialState.TurnState.teamId;
	auto aiUnit = view(getUnitUnderCursor | ignoreOptional, initialState);
	aiTeamId = aiUnit.teamId;

	TMap<int, TSet<int>> TeamIdToUnitIds;

	for (TTuple<int, FDungeonLogicUnit> LoadedUnit : initialState.Map.LoadedUnits)
	{
		int UnitTeamId = LoadedUnit.Value.teamId;
		auto& TeamSet = TeamIdToUnitIds.FindOrAdd(UnitTeamId);
		TeamSet.Add(LoadedUnit.Key);
	}

	auto aiUnitIds = TeamIdToUnitIds.FindAndRemoveChecked(aiTeamId);
	auto enemyIds = Algo::TransformAccumulate(TeamIdToUnitIds, [](auto& x) { return x.Value; }, TSet<int>(),
	                                          [](auto&& acc, auto&& y)
	                                          {
		                                          acc.Append(y);
		                                          return acc;
	                                          });

	auto localView = [&](auto... lenses)
	{
		constexpr auto numberOfParams = sizeof...(lenses);
		if constexpr (numberOfParams > 1)
		{
			return lager::view(fan(lenses...), initialState);
		}
		else
		{
			return lager::view(lenses..., initialState);
		}
	};

	using TClosestDistanceAndUnitId = TTuple<int, int>;

	//What the fuck
	TArray<TArray<TClosestDistanceAndUnitId>> closestUnitMap = TArray<TArray<TClosestDistanceAndUnitId>>();
	auto rowInit = TArray<TClosestDistanceAndUnitId>();
	rowInit.Init(TClosestDistanceAndUnitId(), initialState.Map.Height);
	closestUnitMap.Init(rowInit, initialState.Map.Height);

	for (int i = 0; i < initialState.Map.Width; i++)
	{
		for (int j = 0; j < initialState.Map.Height; j++)
		{
			auto thisPt = FIntPoint{i, j};
			TClosestDistanceAndUnitId minTuple = {INT_MAX, -1};

			for (int enemyId : enemyIds)
			{
				auto currentTuple = TClosestDistanceAndUnitId{
					manhattanDistance(thisPt - localView(unitIdToPosition(enemyId))), enemyId
				};
				minTuple = [](auto&& a, auto&& b) { return TLess<>{}(get<0>(a), get<0>(b)); }(currentTuple, minTuple)
					           ? currentTuple
					           : minTuple;
			}

			closestUnitMap[i][j] = minTuple;
		}
	}


	// auto AIUnitId = aiUnit.Id;

	for (int aiUnitId : aiUnitIds)
	{
		const FDungeonWorldState& state = **gm->store;
		using TActionsTuple = TTuple<FMoveAction, TOptional<FCombatAction>>;
		TArray<TActionsTuple> actions;

		auto [aiPosition,aiUnitData] = lager::view(fan(unitIdToPosition(aiUnitId), unitDataLens(aiUnitId)),
		                                           initialState);
		auto interactablePoints = manhattanReachablePoints(
			state.Map.Width - 1,
			state.Map.Height - 1,
			aiUnitData->Movement,
			aiPosition);

		for (FIntPoint movePoint : interactablePoints)
		{
			auto possibleUnit = view(getUnitAtPointLens(movePoint), state);
			if (possibleUnit.IsSet() && *possibleUnit != aiUnitId)
			{
				continue;
			}
			auto zaTup = TActionsTuple(FMoveAction(aiUnitId, movePoint), TOptional<FCombatAction>());
			auto& [moveA,optCombat] = zaTup;
			const auto& [distance, unitId] = closestUnitMap[movePoint.X][movePoint.Y];

			if (distance <= aiUnitData->attackRange)
			{
				optCombat = FCombatAction(aiUnitId, localView(unitDataLens(unitId))->Id, aiUnitData->damage);
			}
			actions.Add(zaTup);
		}

		Algo::Sort(actions, [&](TActionsTuple& a, TActionsTuple& b)
		{
			const auto& aPoint = a.Key.Destination;
			const auto& bPoint = b.Key.Destination;

			if (TEqualTo()(a.Value.IsSet(), b.Value.IsSet()))
			{
				if (TEqualTo()(closestUnitMap[aPoint.X][aPoint.Y].Key, closestUnitMap[bPoint.X][bPoint.Y].Key))
					return TLess<>()((aiPosition - a.Key.Destination).Size(), (aiPosition - b.Key.Destination).Size());
				else
					return TLess<>()(closestUnitMap[aPoint.X][aPoint.Y].Key, closestUnitMap[bPoint.X][bPoint.Y].Key);
			}
			else
			{
				return !TLess<>{}(a.Value.IsSet(), b.Value.IsSet());
			}
		});

		auto combatToDisp = TSet<FIntPoint>{};
		auto moveToDisp = TSet<FIntPoint>{};

		for (TActionsTuple Action : actions)
		{
			auto& [moveAct, combatAct] = Action;

			// if ([](TActionsTuple x) { return x.Value.IsSet(); }(Action))
			if (combatAct.IsSet())
			{
				combatToDisp.Add(moveAct.Destination);
				// ([](TActionsTuple x) { return x.Key.Destination; }(Action));
			}
			else
			{
				moveToDisp.Add(moveAct.Destination);
			}
		}

		// gm->TileVisualizationComponent->Clear();
		// gm->TileVisualizationComponent->ShowTiles(combatToDisp, FLinearColor::Green);
		// gm->TileVisualizationComponent->ShowTiles(moveToDisp, FLinearColor::Yellow);

		[&](TActionsTuple tuple)
		{
			gm->Dispatch(MoveTemp(tuple.Key));

			if (tuple.Value.IsSet())
				gm->Dispatch(MoveTemp(*tuple.Value));
			else
				gm->Dispatch(FWaitAction{tuple.Key.InitiatorId});
		}(MoveTemp(actions[0]));
	}

	return FReply::Handled();
}

ADungeonGameModeBase::ADungeonGameModeBase(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer)
{
	static ConstructorHelpers::FClassFinder<UDungeonMainWidget> HandlerClass(
		TEXT("/Game/Blueprints/Widgets/MainWidget"));
	static ConstructorHelpers::FClassFinder<AStaticMeshActor> BlockingBorderActor(
		TEXT("/Game/untitled_category/untitled_asset/BlockingBorderActor"));

	MainWidgetClass = HandlerClass.Class;
	BlockingBorderActorClass = BlockingBorderActor.Class;

	PrimaryActorTick.bCanEverTick = true;
	UnitTable = ObjectInitializer.CreateDefaultSubobject<UDataTable>(this, "Default");
	UnitTable->RowStruct = FDungeonLogicUnit::StaticStruct();
}

#define DUNGEON_MOVE_LAMBDA(_x) _x = MoveTemp(_x)

template <typename Fn>
auto TapReducer(Fn&& effectFunction)
{
	return [&](auto next)
	{
		return [&,next](auto action,
		                auto&& model,
		                auto&& reducer,
		                auto&& loop,
		                auto&& deps,
		                auto&& tags)
		{
			return next(action,
			            LAGER_FWD(model),
			            [reducer, effectFunction](auto m, auto a) -> decltype(reducer(LAGER_FWD(m), LAGER_FWD(a)))
			            {
				            auto&& [updatedModel, oldEffect] = reducer(LAGER_FWD(m), LAGER_FWD(a));
				            return {
					            updatedModel,
					            [ DUNGEON_MOVE_LAMBDA(oldEffect), DUNGEON_MOVE_LAMBDA(a), effectFunction ]
				            (auto&& ctx)
					            {
						            oldEffect(DUNGEON_FOWARD(ctx));
						            effectFunction(a);
					            }
				            };
			            },
			            LAGER_FWD(loop),
			            LAGER_FWD(deps),
			            LAGER_FWD(tags));
		};
	};
}

static bool ShouldShowProperty(const FPropertyAndParent& PropertyAndParent, bool bHaveTemplate)
{
	const FProperty& Property = PropertyAndParent.Property;

	if (bHaveTemplate)
	{
		const UClass* PropertyOwnerClass = Property.GetOwner<const UClass>();
		const bool bDisableEditOnTemplate = PropertyOwnerClass
			&& PropertyOwnerClass->IsNative()
			&& Property.HasAnyPropertyFlags(CPF_DisableEditOnTemplate);
		if (bDisableEditOnTemplate)
		{
			return false;
		}
	}
	return true;
}

TSharedRef<SWindow> CreateFloatingDetailsView(const TArray<UViewingModel*>& InObjects,
                                              bool bIsLockable)
{
	TSharedRef<SWindow> NewSlateWindow = SNew(SWindow)
		.Title(NSLOCTEXT("PropertyEditor", "WindowTitle", "Property Editor"))
		.ClientSize(FVector2D(400, 550));

	// If the main frame exists parent the window to it
	TSharedPtr<SWindow> ParentWindow;
	if (FModuleManager::Get().IsModuleLoaded("MainFrame"))
	{
		IMainFrameModule& MainFrame = FModuleManager::GetModuleChecked<IMainFrameModule>("MainFrame");
		ParentWindow = MainFrame.GetParentWindow();
	}

	if (ParentWindow.IsValid())
	{
		// Parent the window to the main frame 
		FSlateApplication::Get().AddWindowAsNativeChild(NewSlateWindow, ParentWindow.ToSharedRef());
	}
	else
	{
		FSlateApplication::Get().AddWindow(NewSlateWindow);
	}

	FDetailsViewArgs Args;
	Args.bHideSelectionTip = true;
	Args.bLockable = bIsLockable;

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>(
		"PropertyEditor");
	TSharedRef<IDetailsView> DetailView = PropertyEditorModule.CreateDetailView(Args);

	bool bHaveTemplate = false;
	for (int32 i = 0; i < InObjects.Num(); i++)
	{
		if (InObjects[i] != NULL && InObjects[i]->IsTemplate())
		{
			bHaveTemplate = true;
			break;
		}
	}

	DetailView->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateStatic(&ShouldShowProperty, bHaveTemplate));

	DetailView->SetObjects(TArray<UObject*>(InObjects));

	auto wew = InObjects[0];

	NewSlateWindow->SetContent(
		// SNew(SBorder)
		SNew(SBorder)
		// .BorderImage(FEditorStyle::GetBrush(TEXT("PropertyWindow.WindowBorder")))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()[
				DetailView
			]
			+ SVerticalBox::Slot()[
				SNew(SButton).OnClicked_UObject(wew, &UViewingModel::GenerateMoves)
			]
		]
	);

	return NewSlateWindow;
}

void ADungeonGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::Get().GetModuleChecked<FPropertyEditorModule>(
		"PropertyEditor");

	ViewingModel = NewObject<UViewingModel>(this);
	ViewingModel->gm = this;
	viewingModels.Add(ViewingModel);
	ModelViewingWindow = CreateFloatingDetailsView(viewingModels, false);
	ModelViewingWindow->ShowWindow();

	TArray<FDungeonLogicUnitRow*> unitsArray;
	UnitTable->GetAllRows(TEXT(""), unitsArray);
	TMap<int, FDungeonLogicUnitRow> loadedUnitTypes;
	for (auto unit : unitsArray)
	{
		loadedUnitTypes.Add(unit->unitData.Id, *unit);
	}

	auto Filename = FPaths::Combine(FPaths::ProjectDir(), TEXT("MapLevel.json"));

	FString readJsonFile;
	FFileHelper::LoadFileToString(readJsonFile, ToCStr(Filename));

	FDungeonLogicMap testa;
	FJsonObjectConverter::JsonObjectStringToUStruct(readJsonFile, &testa);

	auto hey =
		FDungeonLogicMap{
			.Width = 10, .Height = 10,
			.TileAssignment = {
				{{1, 2}, 12},
				{{3, 1}, 1}
			}
		};

	FString output;
	FJsonObjectConverter::UStructToJsonObjectString(hey, output);

	FJsonObjectConverter::JsonObjectStringToUStruct(output, &hey);

	FFileHelper::SaveStringToFile(output, ToCStr(Filename));

	FString StuffR;
	FString TheStr = FPaths::Combine(FPaths::ProjectDir(), TEXT("theBoard.csv"));
	if (FFileHelper::LoadFileToString(StuffR, ToCStr(TheStr)))
	{
		const FCsvParser Parser(MoveTemp(StuffR));
		const auto& Rows = Parser.GetRows();
		int32 height = Rows.Num();

		Game.Map.LoadedTiles.Add(1, {1, "Grass", 1});

		for (int i = 0; i < height; i++)
		{
			int32 width = Rows[i].Num();
			Game.Map.Height = height;
			Game.Map.Width = width;
			Game.TurnState.teamId = 1;
			for (int j = 0; j < width; j++)
			{
				int dataIndex = i * width + j;
				auto value = FCString::Atoi(Rows[i][j]);

				Game.Map.TileAssignment.Add({i, j}, 1);

				if (loadedUnitTypes.Contains(value))
				{
					auto zaRow = loadedUnitTypes[value];
					auto zaunit = zaRow.unitData;
					zaunit.Id = dataIndex;
					zaunit.Name = FString::FormatAsNumber(dataIndex);
					Game.Map.LoadedUnits.Add(zaunit.Id, zaunit);

					FIntPoint positionPlacement{i, j};
					Game.Map.UnitAssignment.Add(positionPlacement, dataIndex);

					UClass* UnrealActor = zaRow.UnrealActor.Get();
					FTransform Transform = FTransform{FRotator::ZeroRotator, FVector::ZeroVector};
					auto unitActor = GetWorld()->SpawnActor<ADungeonUnitActor>(UnrealActor, Transform);
					unitActor->id = zaunit.Id;

					Game.unitIdToActor.Add(zaunit.Id, unitActor);
					unitActor->SetActorLocation(TilePositionToWorldPoint(positionPlacement));
				}
			}
		}
	}

	const auto TapUpdateViewingModel = [this](auto next)
	{
		return [this,next](auto action,
		                   auto&& model,
		                   auto&& reducer,
		                   auto&& loop,
		                   auto&& deps,
		                   auto&& tags)
		{
			return next(action,
			            LAGER_FWD(model),
			            [reducer, this](auto&& m, auto&& a) -> decltype(reducer(LAGER_FWD(m), LAGER_FWD(a)))
			            {
				            auto&& [updatedModel, oldEffect] = reducer(LAGER_FWD(m), LAGER_FWD(a));
				            this->ViewingModel->currentModel = updatedModel;
				            return {updatedModel, oldEffect};
			            },
			            LAGER_FWD(loop),
			            LAGER_FWD(deps),
			            LAGER_FWD(tags));
		};
	};

	store = MakeUnique<TDungeonStore>(TDungeonStore(lager::make_store<TStoreAction>(
		FHistoryModel(this->Game),
		lager::with_manual_event_loop{},
		WithUndoReducer(WorldStateReducer),
		TapReducer([&](TDungeonAction action)
		{
			this->DungeonActionDispatched.Broadcast(action);
			Visit([&]<typename T0>(T0 x)
			{
				using TVisitType = typename TDecay<T0>::Type;
				if constexpr (TIsInTypeUnion<TVisitType, FMoveAction, FCombatAction>::Value )
				{
					FString outJson;
					FJsonObjectConverter::UStructToJsonObjectString<T0>(x, outJson);
					loggingStrings.Add(outJson);
				}
			}, action);
		}),
		TapUpdateViewingModel
	)));

	for (auto UnitIdToActor : static_cast<const FDungeonWorldState>(store->get()).unitIdToActor)
	{
		UnitIdToActor.Value->HookIntoStore();
	}

	FActorSpawnParameters params;
	params.Name = FName("BoardVisualizationActor");
	auto tileShowPrefab = GetWorld()->SpawnActor<ATileVisualizationActor>(TileShowPrefab, FVector::ZeroVector,
	                                                                      FRotator::ZeroRotator, params);

	MovementVisualization = tileShowPrefab->MovementVisualizationComponent;
	TileVisualizationComponent = tileShowPrefab->TileVisualizationComponent;

	MainWidget = CreateWidget<UDungeonMainWidget>(this->GetWorld()->GetFirstPlayerController(), MainWidgetClass);
	MainWidget->AddToViewport();

	auto InAttribute = FCoreStyle::GetDefaultFontStyle("Bold", 30);
	InAttribute.OutlineSettings.OutlineSize = 3.0;
	style = FCoreStyle::Get().GetWidgetStyle<FTextBlockStyle>("NormalText");
	style.SetColorAndOpacity(FSlateColor(FLinearColor::White));

	auto AttributeDisplay = [&](FString&& AttributeName, auto&& methodPointer) -> decltype(SVerticalBox::Slot())
	{
		return MoveTemp(SVerticalBox::Slot().AutoHeight()[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			[
				SNew(STextBlock)
           .TextStyle(&style)
           .Font(InAttribute)
           .Text(FText::FromString(DUNGEON_FOWARD(AttributeName)))
			]
			+ SHorizontalBox::Slot()
			[
				SNew(STextBlock)
           .TextStyle(&style)
           .Font(InAttribute)
           .Text_Lambda(DUNGEON_FOWARD(methodPointer))
			]
		]);
	};

	auto unitPropertyToText = [&](auto&& getter)
	{
		return [this,getter = DUNGEON_FOWARD(getter)]
		{
			const auto& unitUnderCursor = lager::view(getUnitUnderCursor, store->get());
			return unitUnderCursor.IsSet() ? FCoerceToFText::Value(getter(unitUnderCursor.GetValue())) : FText();
		};
	};

	MainWidget->UnitDisplay->SetContent(
		SNew(SVerticalBox)
		+ AttributeDisplay("Name", unitPropertyToText([](const FDungeonLogicUnit x) { return x.Name; }))
		+ AttributeDisplay("HitPoints", unitPropertyToText([](const FDungeonLogicUnit x) { return x.HitPoints; }))
		+ AttributeDisplay("Movement", unitPropertyToText([](const FDungeonLogicUnit x) { return x.Movement; }))
		+ AttributeDisplay("Turn ID", [&] { return GetCurrentTurnId(); })
		+ SVerticalBox::Slot().AutoHeight()[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			[
				SNew(STextBlock)
                     .TextStyle(&style)
                     .Font(InAttribute)
                     .Text(FText::FromString("Context Interaction"))
			]
			+ SHorizontalBox::Slot()
			[
				SNew(STextBlock)
                     .TextStyle(&style)
                     .Font(InAttribute)
                     .Text_Lambda([&]()-> auto
				                {
					                return Visit([&](auto&& context)
					                {
						                return FText::FromString(
							                (TDecay<decltype(context)>::Type::StaticStruct()->GetName()));
					                }, UseViewState(interactionContextLens));
				                })
			]
		]
	);

	this->SingleSubmitHandler = static_cast<USingleSubmitHandler*>(this->AddComponentByClass(
		USingleSubmitHandler::StaticClass(), false, FTransform(), false));
}

void ADungeonGameModeBase::Tick(float deltaTime)
{
	Super::Tick(deltaTime);

	if (this->AnimationQueue.IsEmpty())
		return;

	(*this->AnimationQueue.Peek())->TickTimeline(deltaTime);
}

void ADungeonGameModeBase::Dispatch(TDungeonAction&& unionAction)
{
	store->dispatch(unionAction);
}

FText ADungeonGameModeBase::GetCurrentTurnId() const
{
	return FCoerceToFText::Value(lager::view(turnStateLens, *this).teamId);
}