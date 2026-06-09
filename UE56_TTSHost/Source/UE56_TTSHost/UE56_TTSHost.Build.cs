// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class UE56_TTSHost : ModuleRules
{
	public UE56_TTSHost(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "LocalTTS" });

		PrivateDependencyModuleNames.AddRange(new string[] { });
	}
}
