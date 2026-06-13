#pragma once

#include "CoreMinimal.h"

class UWidgetBlueprint;

class FUIJsonBridgeImporter
{
public:
	static bool ImportWidgetBlueprint(UWidgetBlueprint* WidgetBlueprint, const FString& InputFilePath, FText& OutError);
};
