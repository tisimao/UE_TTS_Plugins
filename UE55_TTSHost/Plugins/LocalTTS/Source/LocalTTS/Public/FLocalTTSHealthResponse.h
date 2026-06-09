// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FLocalTTSHealthResponse.generated.h"

USTRUCT(BlueprintType)
struct LOCALTTS_API FLocalTTSHealthResponse
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS")
	bool bOk = false;

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS")
	FString Service;

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS")
	FString Status;

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS")
	FString Model;

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS")
	TArray<FString> SupportedModes;
};
