// Copyright Epic Games, Inc. All Rights Reserved.

#include "Dungeon.h"
#include "Modules/ModuleManager.h"

#include "CoreMinimal.h"
#include "Misc/Paths.h"
#include "ShaderCore.h"
#include "Logic/DungeonEditor.h"
#include "Styling/StarshipCoreStyle.h"
#include "Widgets/Docking/SDockTab.h"

class FModuleThing : public FDefaultGameModuleImpl
{
  virtual void StartupModule() override;
  virtual void ShutdownModule() override;
};

IMPLEMENT_PRIMARY_GAME_MODULE(FModuleThing, Dungeon, "Dungeon");

static TSharedPtr<FButtonStyle> whatThe;

// decltype(auto) SomeShit(const FSpawnTabArgs& Args)
// {
// 	auto wew = FStarshipCoreStyle::GetCoreStyle().GetWidgetStyle<FButtonStyle>("Button");
// 	auto hey = FAppStyle::Get().GetWidgetStyle<FButtonStyle>("PrimaryButton");
// 	auto wow = MakeShared<FButtonStyle>(FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button"));
// 	wow->Normal.ImageSize = {32,32};
// 	whatThe = wow;
//
// 	TSharedPtr<SDockTab> MajorTab;
// 	SAssignNew(MajorTab, SDockTab)
// 	.TabRole(ETabRole::MajorTab)[SNew(SDungeonEditor)];
// 		
// 	return MajorTab.ToSharedRef();
// }
//
void FModuleThing::StartupModule()
{
  FString ShaderDirectory = FPaths::Combine(FPaths::ProjectDir(), TEXT("Shaders"));
  AddShaderSourceDirectoryMapping("/Project", ShaderDirectory);
}

void FModuleThing::ShutdownModule()
{
  FDefaultGameModuleImpl::ShutdownModule();
}
