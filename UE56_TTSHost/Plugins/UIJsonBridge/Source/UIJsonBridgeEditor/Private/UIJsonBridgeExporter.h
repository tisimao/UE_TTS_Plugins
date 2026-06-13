#pragma once

#include "CoreMinimal.h"

class UWidgetBlueprint;

enum class EUIJsonBridgeExportProfile : uint8
{
	LayoutOnly,
	Interaction,
	Full
};

class FUIJsonBridgeExporter
{
public:
	static bool ExportWidgetBlueprint(UWidgetBlueprint* WidgetBlueprint, const FString& OutputFilePath, FText& OutError);
	static bool ExportWidgetBlueprint(UWidgetBlueprint* WidgetBlueprint, const FString& OutputFilePath, EUIJsonBridgeExportProfile Profile, FText& OutError);
	static FString MakeDefaultExportFilePath(const UWidgetBlueprint* WidgetBlueprint, EUIJsonBridgeExportProfile Profile = EUIJsonBridgeExportProfile::Full);
	static FText GetExportProfileDisplayName(EUIJsonBridgeExportProfile Profile);
};
