// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Dungeon : ModuleRules
{
	public Dungeon(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

    PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore" });

    PrivateDependencyModuleNames.AddRange(new string[] { "Json", "JsonUtilities" });

    // Uncomment if you are using Slate UI
    PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore", "UMG" });

    PrivateIncludePaths.AddRange(new string[] { "Dungeon/ThirdParty" }); 
    Definitions.AddRange(new string[] { "IMMER_NO_FREE_LIST"     , "IMMER_NO_THREAD_SAFETY" , "IMMER_NO_THREAD_SAFETY" , }); 

    // Uncomment if you are using online features
    // PrivateDependencyModuleNames.Add("OnlineSubsystem");

    // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
  }
}
