// Copyright Epic Games, Inc. All Rights Reserved.

#include "Dungeon.h"
#include "Modules/ModuleManager.h"

#include "CoreMinimal.h"
#include "Misc/Paths.h"
#include "ShaderCore.h"

class FModuleThing : public FDefaultGameModuleImpl
{
  virtual void StartupModule() override;
  virtual void ShutdownModule() override;
};

IMPLEMENT_PRIMARY_GAME_MODULE(FModuleThing, Dungeon, "Dungeon");

void FModuleThing::StartupModule()
{
  FString ShaderDirectory = FPaths::Combine(FPaths::ProjectDir(), TEXT("Shaders"));
  AddShaderSourceDirectoryMapping("/Project", ShaderDirectory);
}

void FModuleThing::ShutdownModule()
{
  FDefaultGameModuleImpl::ShutdownModule();
}
