// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class DungeonEditor : ModuleRules
{
	public DungeonEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		CppStandard = CppStandardVersion.Cpp20;
		OptimizeCode = CodeOptimization.Never;
		
		PublicDependencyModuleNames.AddRange(
			new string[] { "Dungeon", 
				//"DataTableEditor"
				//, "UnrealEd" 
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[] {"UnrealEd", 
                                "DataTableEditor", 
								"Core",
                				"CoreUObject",
                                "Json",
                                "JsonUtilities",
                				//"ApplicationCore",
                				//"Engine",
                                //"InputCore",
                				"Slate",
                				"SlateCore",
                                //"EditorStyle",
                                //"PropertyEditor",
                				//"EditorFramework",
                				//"Json"
                				
				// "Core", "Slate", "SlateCore", "UMG", "UMGEditor", "CoreUObject" 
			}
		);

		PrivateIncludePathModuleNames.AddRange( new string[] { } );

		PrivateIncludePaths.AddRange(new string[] {$"../Source/Dungeon/ThirdParty", "../Source/Dungeon"});
		
		// PublicIncludePaths.AddRange(new string[] {$"../Dungeon/ThirdParty", "Dungeon"});

		DynamicallyLoadedModuleNames.AddRange( new string[] { } );
	}
}