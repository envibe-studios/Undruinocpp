// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Unduinocpp : ModuleRules
{
	public Unduinocpp(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Allow #include "AI/..." and nested BT headers from Source/Unduinocpp
		PublicIncludePaths.Add(ModuleDirectory);
	
		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"NetCore",
			"AIModule",
			"GameplayTasks",
			"NavigationSystem",
			"GameplayTags"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { "Sockets", "Networking" });

		// DualJoystickTankInputComponent polls DirectInput joysticks via winmm (Joy0/Joy1).
		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			PublicSystemLibraries.Add("Winmm.lib");
		}

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
