// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"

#include "ULocalTTSSettings.generated.h"

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "LocalTTS"))
class LOCALTTS_API ULocalTTSSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Service")
	FString ServiceBaseUrl = TEXT("http://127.0.0.1:50021");

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Service")
	FString ServiceRelativeRoot = TEXT("Services/tts_service");

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Service")
	FString PythonRelativePath = TEXT(".venv/Scripts/python.exe");

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Service")
	FString RunServerScriptName = TEXT("run_server.py");

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Health Poll", meta = (ClampMin = "1"))
	int32 MaxHealthPollCount = 30;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Health Poll", meta = (ClampMin = "0.1"))
	float HealthPollIntervalSeconds = 1.0f;
};
