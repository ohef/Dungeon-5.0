#pragma once

#include "Dungeon.h"
#include "Misc/AutomationTest.h"
#include "MapEditorState.hpp"
#include "Components/Button.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMapEditorStateTest, "Dungeon.Core.MapEditor", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FMapEditorStateTest::RunTest(const FString& Parameters)
{
	auto previous = RenderMapEditorState({.CurrentMap = FDungeonLogicMap{.UnitAssignment = {
		{{1, 2}, 1},
		{{2, 3}, 2},
		{{1, 1}, 3}
	}}});
	
	auto next = RenderMapEditorState({.CurrentMap = FDungeonLogicMap{.UnitAssignment = {
		{{1, 2}, 1},
	}}});

	UGridPanel* panel = NewObject<UGridPanel>();
	panel->AddChildToGrid(NewObject<UButton>(), 1, 2);
	panel->AddChildToGrid(NewObject<UButton>(), 2, 3);
	panel->AddChildToGrid(NewObject<UButton>(), 1, 1);

	Reconcile(previous, next, *panel);
	
	return true;
}
#endif