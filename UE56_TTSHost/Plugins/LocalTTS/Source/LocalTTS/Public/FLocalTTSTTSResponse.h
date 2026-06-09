// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FLocalTTSTTSResponse.generated.h"

USTRUCT(BlueprintType)
struct LOCALTTS_API FLocalTTSTTSResponse
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS|Response", meta = (DisplayName = "OK", ToolTip = "Whether the TTS request succeeded."))
	bool bOk = false;

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS|Response", meta = (DisplayName = "Request ID", ToolTip = "Service-side request id. Use this to match UE logs with service logs."))
	FString RequestId;

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS|Response", meta = (DisplayName = "Mode", ToolTip = "OmniVoice mode used by the service."))
	FString Mode;

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS|Response", meta = (DisplayName = "WAV Path", ToolTip = "Generated wav file path on local disk."))
	FString WavPath;

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS|Response", meta = (DisplayName = "Sample Rate", ToolTip = "Generated audio sample rate."))
	int32 SampleRate = 0;

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS|Response", meta = (DisplayName = "Duration MS", ToolTip = "Service-side synthesis duration in milliseconds."))
	int32 DurationMs = 0;

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS|Response|Error", meta = (DisplayName = "Error Code", ToolTip = "Service-side error code when the request failed."))
	FString ErrorCode;

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS|Response|Error", meta = (DisplayName = "Error Message", ToolTip = "Readable error message when the request failed."))
	FString ErrorMessage;
};
