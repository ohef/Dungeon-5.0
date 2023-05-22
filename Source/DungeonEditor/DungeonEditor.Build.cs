// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class DungeonEditor : ModuleRules
{
  public DungeonEditor(ReadOnlyTargetRules Target) : base(Target)
  {
    CppStandard = CppStandardVersion.Cpp20;
    OptimizeCode = CodeOptimization.Never;

    PublicDependencyModuleNames.AddRange(
        new string[] { "Dungeon" }
        );

    PrivateDependencyModuleNames.AddRange(
        new string[] {"UnrealEd", 
        "Engine",
        "DataTableEditor", 
        "Core",
        "CoreUObject",
        "Json",
        "JsonUtilities",
        "Slate",
        "SlateCore", 
        "InputCore",
        "EditorScriptingUtilities",
        "EditorFramework", "Blutility",
        }
        );

    PrivateIncludePathModuleNames.AddRange( new string[] { } );

    PrivateIncludePaths.AddRange(new string[] {$"../Source/Dungeon/ThirdParty", "../Source/Dungeon"});

    DynamicallyLoadedModuleNames.AddRange( new string[] { } );
  }
}
