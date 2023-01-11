#include "DungeonEditor.h"
#include "CoreMinimal.h"
#include "Styling/StarshipCoreStyle.h"
#include "DungeonMapEditor.h"

decltype(auto) SomeShit(const FSpawnTabArgs& Args)
{
	auto wew = FStarshipCoreStyle::GetCoreStyle().GetWidgetStyle<FButtonStyle>("Button");
	auto hey = FAppStyle::Get().GetWidgetStyle<FButtonStyle>("PrimaryButton");
	auto wow = MakeShared<FButtonStyle>(FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button"));
	wow->Normal.ImageSize = {32, 32};

	TSharedPtr<SDockTab> MajorTab;
	SAssignNew(MajorTab, SDockTab)
		.TabRole(ETabRole::MajorTab)[SNew(SDungeonEditor)];

	return MajorTab.ToSharedRef();
}

inline void FDungeonEditorModule::StartupModule()
{
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(FName(TEXT("SUP")), FOnSpawnTab::CreateStatic(&SomeShit))
	                        .SetDisplayName(NSLOCTEXT("MapEditor", "TabTitle", "Map Editor"))
	                        .SetTooltipText(NSLOCTEXT("MapEditor", "TooltipText", "Work you fucking piece of SHIT"));
	// .SetGroup(WorkspaceMenu::GetMenuStructure().GetToolsCategory())
	// .SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "ClassIcon.UserDefinedStruct"));
}

inline void FDungeonEditorModule::ShutdownModule()
{
}

IMPLEMENT_GAME_MODULE(FDungeonEditorModule, DungeonEditor);
