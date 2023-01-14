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
#include "Engine/DataTable.h"
#include "GameFramework/GameStateBase.h"
#include "lager/event_loop/manual.hpp"
#include "lager/store.hpp"
#include "lager/util.hpp"
#include "Lenses/model.hpp"
#include "Logic/SimpleTileGraph.h"
#include "Serialization/Csv/CsvParser.h"

ADungeonGameModeBase::ADungeonGameModeBase(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer)
{
	static ConstructorHelpers::FClassFinder<UDungeonMainWidget> HandlerClass(
		TEXT("/Game/Blueprints/Widgets/MainWidget"));
	MainWidgetClass = HandlerClass.Class;

	PrimaryActorTick.bCanEverTick = true;
	UnitTable = ObjectInitializer.CreateDefaultSubobject<UDataTable>(this, "Default");
	UnitTable->RowStruct = FDungeonLogicUnit::StaticStruct();
}

#define DUNGEON_MOVE_LAMBDA(_x) _x = MoveTemp(_x)

auto WithGlobalEventMiddleware(const ADungeonGameModeBase& gameMode)
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
			            [reducer, &gameMode](auto m, auto a) -> decltype(reducer(LAGER_FWD(m), LAGER_FWD(a)))
			            {
				            auto&& [updatedModel, oldEffect] = reducer(LAGER_FWD(m), LAGER_FWD(a));
				            return {
					            updatedModel, [ DUNGEON_MOVE_LAMBDA(oldEffect), DUNGEON_MOVE_LAMBDA(a), &gameMode ]
				            (auto&& ctx)
					            {
						            oldEffect(DUNGEON_FOWARD(ctx));
						            gameMode.DungeonActionDispatched.Broadcast(a);
					            }
				            };
			            },
			            LAGER_FWD(loop),
			            LAGER_FWD(deps),
			            LAGER_FWD(tags));
		};
	};
}

void ADungeonGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::Get().GetModuleChecked<FPropertyEditorModule>(
		"PropertyEditor");

	ViewingModel = NewObject<UViewingModel>(this);
	viewingModels.Add(ViewingModel);
	ModelViewingWindow = PropertyEditorModule.CreateFloatingDetailsView(viewingModels, false);
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

	store = MakeUnique<TDungeonStore>(TDungeonStore(lager::make_store<TStoreAction>(
		FHistoryModel(this->Game),
		lager::with_manual_event_loop{},
		WithUndoReducer(WorldStateReducer),
		WithGlobalEventMiddleware(*this),
		[this](auto next)
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
				            	// this->ViewingModel->currentModel = updatedModel;
					            return { updatedModel, oldEffect };
				            },
				            LAGER_FWD(loop),
				            LAGER_FWD(deps),
				            LAGER_FWD(tags));
			};
		}
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
