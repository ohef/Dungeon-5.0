// Copyright Epic Games, Inc. All Rights Reserved.

#include "Dungeon.h"
#include "DungeonGameModeBase.h"
#include "DungeonReducer.hpp"
#include "DungeonUnitActor.h"
#include "Containers/UnrealString.h"
#include "lager/event_loop/manual.hpp"
#include "Misc/AutomationTest.h"
#include "Serialization/Csv/CsvParser.h"


#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDungeonStoreTest, "Dungeon.Core", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FDungeonStoreTest::RunTest(const FString& Parameters)
{
	auto model = FHistoryModel();
	auto& Game = model.CurrentState();

	//(unitData=(Id=66,damage=2,HitPoints=20,state=Free,Name="66",Movement=3,teamId=1,attackRange=1),UnrealActor=BlueprintGeneratedClass'"/Game/Unit/Unit4.Unit4_C"')
	//(unitData=(Id=67,damage=2,HitPoints=13,state=Free,Name="67",Movement=8,teamId=2,attackRange=1),UnrealActor=BlueprintGeneratedClass'"/Game/Unit/Unit3.Unit3_C"')
	
	// TArray<FDungeonLogicUnitRow*> unitsArray;
	// UnitTable->GetAllRows(TEXT(""), unitsArray);
	TMap<int, FDungeonLogicUnitRow> loadedUnitTypes;
	loadedUnitTypes.Add(66, FDungeonLogicUnitRow(FDungeonLogicUnit(66, 2, 20, Free, "66", 3, 1, 1)));
	loadedUnitTypes.Add(67, FDungeonLogicUnitRow(FDungeonLogicUnit(67, 2, 13, Free, "67", 8, 2, 1)));
	// for (auto unit : unitsArray)
	// {
	// 	loadedUnitTypes.Add(unit->unitData.Id, *unit);
	// }

	FString StuffR;
	FString TheStr = FPaths::Combine(FPaths::ProjectDir(), TEXT("theBoard.csv"));
	if (FFileHelper::LoadFileToString(StuffR, ToCStr(TheStr)))
	{
		const FCsvParser Parser(MoveTemp(StuffR));
		const auto& Rows = Parser.GetRows();
		int32 height = Rows.Num();

		Game.map.loadedTiles.Add(1, {1, "Grass", 1});

		for (int i = 0; i < height; i++)
		{
			int32 width = Rows[i].Num();
			Game.map.Height = height;
			Game.map.Width = width;
			Game.TurnState.teamId = 1;
			for (int j = 0; j < width; j++)
			{
				int dataIndex = i * width + j;
				auto value = FCString::Atoi(Rows[i][j]);

				Game.map.tileAssignment.Add({i, j}, 1);

				if (loadedUnitTypes.Contains(value))
				{
					auto zaRow = loadedUnitTypes[value];
					auto zaunit = zaRow.unitData;
					zaunit.Id = dataIndex;
					zaunit.Name = FString::FormatAsNumber(dataIndex);
					Game.map.loadedUnits.Add(zaunit.Id, zaunit);

					FIntPoint positionPlacement{i, j};
					Game.map.unitAssignment.Add(positionPlacement, dataIndex);
				}
			}
		}
	}
	
	auto store = MakeUnique<TDungeonStore>(TDungeonStore(lager::make_store<TStoreAction>(
		model,
		lager::with_manual_event_loop{},
		WithUndoReducer(WorldStateReducer)
	)));
	
	// constructors must create equal objects
	{
		store->dispatch(TDungeonAction(TInPlaceType<FCursorPositionUpdated>{}, FIntPoint{1,4}));
		store->dispatch(TDungeonAction(TInPlaceType<FCursorQueryTarget>{}, FIntPoint{1,4}));

		auto GetUnitMenuContext = interactionContextLens
			| unreal_alternative_pipeline<FUnitMenu>;

		auto GetUnitIdFromContext = GetUnitMenuContext
			| map_opt(attr(&FUnitMenu::unitId))
			| value_or(-1);

		int ViewState = lager::view(GetUnitIdFromContext,store->get());
		store->dispatch(
		TDungeonAction(
        		TInPlaceType<FSteppedAction>{},
        		TStepAction(TInPlaceType<FMoveAction>{}, ViewState),
        		TArray {
        			TInteractionContext(TInPlaceType<FSelectingUnitAbilityTarget>{})
        		}));
		
		TestTrue("Should be in SelectingUnitAbilityTarget",
			store->zoom(interactionContextLens).make().get().IsType<FSelectingUnitAbilityTarget>());
			
		store->dispatch(TDungeonAction(TInPlaceType<FCursorQueryTarget>{}, FIntPoint{1,6}));
		
		TestTrue("Should be in UnitMenu", store->zoom(interactionContextLens).make().get().IsType<FUnitMenu>());
		
		store->dispatch(TDungeonAction(TInPlaceType<FBackAction>{}));
		store->dispatch(TDungeonAction(TInPlaceType<FCursorQueryTarget>{}, FIntPoint{1,6}));
		store->dispatch(TDungeonAction(TInPlaceType<FBackAction>{}));
		
		TestTrue("Should be in SelectingUnitAbilityTarget",
			store->zoom(interactionContextLens).make().get().IsType<FSelectingUnitAbilityTarget>());
	}
	
	return true;
}

#endif //WITH_DEV_AUTOMATION_TESTS
