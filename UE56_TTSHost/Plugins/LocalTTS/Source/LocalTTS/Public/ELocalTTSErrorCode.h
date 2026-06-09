// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ELocalTTSErrorCode.generated.h"

UENUM(BlueprintType)
enum class ELocalTTSErrorCode : uint8
{
	None UMETA(DisplayName = "None"),
	InternalError UMETA(DisplayName = "Internal Error"),
	ServiceProcessError UMETA(DisplayName = "Service Process Error"),
	ServiceUnreachable UMETA(DisplayName = "Service Unreachable"),
	HttpError UMETA(DisplayName = "HTTP Error"),
	ParseError UMETA(DisplayName = "Parse Error"),
	RequestValidationFailed UMETA(DisplayName = "Request Validation Failed"),
	ServiceReturnedError UMETA(DisplayName = "Service Returned Error"),
	AlreadyBusy UMETA(DisplayName = "Already Busy"),
	WavFileInvalid UMETA(DisplayName = "WAV File Invalid"),
	WavLoadFailed UMETA(DisplayName = "WAV Load Failed"),
	PlaybackFailed UMETA(DisplayName = "Playback Failed")
};
