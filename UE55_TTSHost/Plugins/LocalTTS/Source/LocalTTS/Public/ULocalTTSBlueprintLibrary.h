// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "FLocalTTSHealthResponse.h"
#include "FLocalTTSTTSResponse.h"
#include "ULocalTTSBlueprintLibrary.generated.h"

UCLASS()
class LOCALTTS_API ULocalTTSBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "LocalTTS", meta = (WorldContext = "WorldContextObject"))
	static bool StartLocalTTS(const UObject* WorldContextObject, FString& ErrorMessage);

	UFUNCTION(BlueprintCallable, Category = "LocalTTS", meta = (WorldContext = "WorldContextObject"))
	static void StopLocalTTS(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "LocalTTS", meta = (WorldContext = "WorldContextObject"))
	static void StopSpeaking(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "LocalTTS", meta = (WorldContext = "WorldContextObject"))
	static bool IsLocalTTSReady(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "LocalTTS", meta = (WorldContext = "WorldContextObject"))
	static bool IsSpeaking(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "LocalTTS", meta = (WorldContext = "WorldContextObject"))
	static FLocalTTSHealthResponse GetLastLocalTTSHealth(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "LocalTTS", meta = (WorldContext = "WorldContextObject"))
	static FString GetLastLocalTTSError(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "LocalTTS", meta = (WorldContext = "WorldContextObject"))
	static FLocalTTSTTSResponse GetLastLocalTTSSpeechResult(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "LocalTTS", meta = (WorldContext = "WorldContextObject"))
	static FString GetLastLocalTTSSpeechError(const UObject* WorldContextObject);

private:
	static class ULocalTTSSubsystem* ResolveSubsystem(const UObject* WorldContextObject);
};
