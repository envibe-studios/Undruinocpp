// Copyright (c) 2026, Srivanth. All Rights Reserved.

using UnrealBuildTool;

public class ScreenBridge : ModuleRules
{
	public ScreenBridge(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
			}
			);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"UMG",
				"Slate",
				"SlateCore",
				"ApplicationCore"
			}
			);
	}
}
