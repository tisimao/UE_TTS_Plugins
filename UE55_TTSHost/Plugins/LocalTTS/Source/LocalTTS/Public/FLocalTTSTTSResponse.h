// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FLocalTTSTTSResponse.generated.h"

USTRUCT(BlueprintType)
struct LOCALTTS_API FLocalTTSTTSResponse
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS")
	bool bOk = false;

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS")
	FString RequestId;

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS")
	FString Mode;

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS")
	FString WavPath;

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS")
	int32 SampleRate = 0;

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS")
	int32 DurationMs = 0;

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS")
	FString ErrorCode;

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS")
	FString ErrorMessage;
};
