// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Dungeon : ModuleRules
{
	public Dungeon(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		CppStandard = CppStandardVersion.Cpp20;
		OptimizeCode = CodeOptimization.Never;

		PublicDependencyModuleNames.AddRange(new string[]
			{"Core", "CoreUObject", "Engine", "InputCore", "RenderCore", "Slate", "SlateCore", "UMG", "MainFrame", 
			});
		PrivateDependencyModuleNames.AddRange(new string[] {
			"Json", "JsonUtilities", "Slate", "SlateCore", "UMG", "WorkspaceMenuStructure", "DataTableEditor", 
		});

		PrivateIncludePaths.AddRange(new string[] {"Dungeon/ThirdParty", "Dungeon"});

		PublicDefinitions.AddRange(new string[]
		{
			"LAGER_DISABLE_STORE_DEPENDENCY_CHECKS",
			// "LAGER_NO_EXCEPTIONS",
			// "ZUG_ENABLE_BOOST=0",
			"ZUG_REDUCE_TAIL_RECURSIVE",
			// "ZUG_VARIANT_BOOST=1",
			"ZUG_VARIANT_STD=1",
			"IMMER_NO_FREE_LIST",
			"IMMER_NO_THREAD_SAFETY",
			"IMMER_NO_THREAD_SAFETY"
		});
	}
}