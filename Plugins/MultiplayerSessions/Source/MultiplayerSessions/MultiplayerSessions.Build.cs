// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MultiplayerSessions : ModuleRules
{
	public MultiplayerSessions(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// add required public include paths here
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// add required private include paths here
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[] // writing names here because this will act as plugin, we need to specify all dependencies so that it can be used in any project
			{ 
				"Core",

				"OnlineSubsystem", // for general online features
				"OnlineSubsystemSteam", // for steam features

				"UMG", // Unreal Motion Graphics, UI related
				
				 // I only knows these stuff were throwing warning during early builds 
				"Slate", // also related to UI?
				"SlateCore",

                "OnlineSubsystemUtils",
                
                "AudioMixer" // for the new voice chat 
			}
		);
			
		
		PrivateDependencyModuleNames.AddRange( // these are not needed to be exposed to the project using this plugin
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
			}
		);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// add modules that loads dynamically here
			}
			);
	}
}
