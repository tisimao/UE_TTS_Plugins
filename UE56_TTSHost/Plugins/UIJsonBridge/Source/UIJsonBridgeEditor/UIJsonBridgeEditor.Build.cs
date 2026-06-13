using UnrealBuildTool;

public class UIJsonBridgeEditor : ModuleRules
{
	public UIJsonBridgeEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"ApplicationCore",
				"AssetRegistry",
				"AutomationController",
				"BlueprintGraph",
				"ContentBrowser",
				"CoreUObject",
				"DesktopPlatform",
				"Engine",
				"Json",
				"JsonUtilities",
				"Slate",
				"SlateCore",
				"ToolMenus",
				"UMG",
				"UMGEditor",
				"UnrealEd"
			}
		);
	}
}
