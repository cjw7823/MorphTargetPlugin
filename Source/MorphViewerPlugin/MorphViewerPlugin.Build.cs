// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MorphViewerPlugin : ModuleRules
{
	public MorphViewerPlugin(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
                "UnrealEd",
                "AdvancedPreviewScene",
                "EditorStyle",
                "inputcore",
                "LevelEditor",
                "AnimationEditor",
                "Persona",
				"SkeletonEditor",
				"PropertyEditor",
				"MorphViewerPluginRuntime",
                "CurveEditor",
                "AnimGraph",
                "SequenceRecorder",
                "Kismet",
                "AssetManagerEditor",
                "EditorWidgets",
                "KismetWidgets",
                "AssetTools",
                "AssetRegistry",
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				// ... add private dependencies that you statically link with here ...	
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
