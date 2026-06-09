// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FLocalTTSSpeakRequest.generated.h"

USTRUCT(BlueprintType)
struct LOCALTTS_API FLocalTTSSpeakRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LocalTTS")
	FString Text;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LocalTTS")
	FString Mode = TEXT("auto");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LocalTTS")
	FString LanguageId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LocalTTS")
	FString ReferenceAudioPath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LocalTTS")
	FString ReferenceText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LocalTTS")
	FString Instruct;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LocalTTS")
	float Duration = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LocalTTS")
	float Speed = 1.0f;
};
