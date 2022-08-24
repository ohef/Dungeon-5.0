// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Dungeon : ModuleRules
{
	public Dungeon(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		CppStandard = CppStandardVersion.Cpp17;

		PublicDependencyModuleNames.AddRange(new string[] {"Core", "CoreUObject", "Engine", "InputCore", "RenderCore", "Slate", "SlateCore", "UMG"});
		PrivateDependencyModuleNames.AddRange(new string[] {"Json", "JsonUtilities", "Slate", "SlateCore", "UMG"});
		OptimizeCode = CodeOptimization.Never;

		PrivateIncludePaths.AddRange(new string[] {"Dungeon/ThirdParty"});
		Definitions.AddRange(new string[] {"IMMER_NO_FREE_LIST", "IMMER_NO_THREAD_SAFETY", "IMMER_NO_THREAD_SAFETY"});
	}
}