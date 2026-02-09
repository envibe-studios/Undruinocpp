// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ArduinoCommunication : ModuleRules
{
	public ArduinoCommunication(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		// Required for UE 5.7
		bUseUnity = false;

		PublicIncludePaths.AddRange(
			new string[] {
			}
		);

		PrivateIncludePaths.AddRange(
			new string[] {
				System.IO.Path.Combine(ModuleDirectory, "../../../..", "Source/Unduinocpp")
			}
		);

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"Sockets",
				"Networking",
				"Slate",
				"SlateCore",
				"RHI",
				"RenderCore"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Unduinocpp"
			}
		);

		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
			}
		);

		// Platform-specific settings for serial port access
		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			// Windows serial port support
		}
		else if (Target.Platform == UnrealTargetPlatform.Linux)
		{
			// Linux serial port support
		}
		else if (Target.Platform == UnrealTargetPlatform.Mac)
		{
			// Mac serial port support
		}
	}
}
