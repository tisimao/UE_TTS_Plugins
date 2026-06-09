// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FLocalTTSHealthResponse.generated.h"

USTRUCT(BlueprintType)
struct LOCALTTS_API FLocalTTSHealthResponse
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS|Health", meta = (DisplayName = "OK", ToolTip = "Whether the local TTS service responded successfully."))
	bool bOk = false;

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS|Health", meta = (DisplayName = "Service", ToolTip = "Service name returned by the local TTS server."))
	FString Service;

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS|Health", meta = (DisplayName = "Status", ToolTip = "Service status, for example ready or loading."))
	FString Status;

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS|Health", meta = (DisplayName = "Model", ToolTip = "Model name currently loaded by the service."))
	FString Model;

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS|Health", meta = (DisplayName = "Supported Modes", ToolTip = "TTS modes supported by the service."))
	TArray<FString> SupportedModes;
};
