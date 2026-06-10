// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class IT_Learns : ModuleRules
{
	public IT_Learns(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate",
			"OnlineSubsystem",
            "OnlineSubsystemUtils",	

            "OnlineSubsystemEOS"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"IT_Learns",
			"IT_Learns/Variant_Horror",
			"IT_Learns/Variant_Horror/UI",
			"IT_Learns/Variant_Shooter",
			"IT_Learns/Variant_Shooter/AI",
			"IT_Learns/Variant_Shooter/UI",
			"IT_Learns/Variant_Shooter/Weapons"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
