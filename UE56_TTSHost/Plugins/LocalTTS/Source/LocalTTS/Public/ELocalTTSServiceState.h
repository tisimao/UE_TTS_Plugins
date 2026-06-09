// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ELocalTTSServiceState.generated.h"

UENUM(BlueprintType)
enum class ELocalTTSServiceState : uint8
{
	Stopped UMETA(DisplayName = "Stopped", ToolTip = "The local TTS service is not running."),
	Starting UMETA(DisplayName = "Starting", ToolTip = "The local TTS service process has been requested and is warming up."),
	Ready UMETA(DisplayName = "Ready", ToolTip = "The local TTS service is reachable and ready."),
	Busy UMETA(DisplayName = "Busy", ToolTip = "The local TTS service is processing a request."),
	Error UMETA(DisplayName = "Error", ToolTip = "The local TTS service or request is in an error state.")
};
