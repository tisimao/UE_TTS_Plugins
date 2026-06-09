// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "ELocalTTSErrorCode.h"
#include "ELocalTTSServiceState.h"
#include "FLocalTTSHealthResponse.h"
#include "FLocalTTSSpeakRequest.h"
#include "FLocalTTSTTSResponse.h"
#include "ULocalTTSBlueprintLibrary.generated.h"

UCLASS()
class LOCALTTS_API ULocalTTSBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "LocalTTS|Request", meta = (DisplayName = "Make Local TTS Auto Request", ToolTip = "Create a LocalTTS request for OmniVoice auto mode. This is the recommended default request."))
	static FLocalTTSSpeakRequest MakeAutoSpeakRequest(const FString& Text, const FString& LanguageId, float Speed);

	UFUNCTION(BlueprintPure, Category = "LocalTTS|Request", meta = (DisplayName = "Make Local TTS Design Request", ToolTip = "Create a LocalTTS request for OmniVoice design mode. Use Instruct to describe the desired voice style."))
	static FLocalTTSSpeakRequest MakeDesignSpeakRequest(const FString& Text, const FString& Instruct, const FString& LanguageId, float Speed);

	UFUNCTION(BlueprintPure, Category = "LocalTTS|Request", meta = (DisplayName = "Make Local TTS Clone Request", ToolTip = "Create a LocalTTS request for OmniVoice clone mode. ReferenceAudioPath should point to a local wav file."))
	static FLocalTTSSpeakRequest MakeCloneSpeakRequest(const FString& Text, const FString& ReferenceAudioPath, const FString& ReferenceText, const FString& LanguageId, float Speed);

	UFUNCTION(BlueprintCallable, Category = "LocalTTS|Request", meta = (DisplayName = "Validate Local TTS Request", ToolTip = "Validate a LocalTTS request before sending it to the local OmniVoice service."))
	static bool ValidateSpeakRequest(const FLocalTTSSpeakRequest& SpeakRequest, FString& ErrorMessage);

	UFUNCTION(BlueprintCallable, Category = "LocalTTS|Service", meta = (WorldContext = "WorldContextObject", DisplayName = "Start Local TTS", ToolTip = "Start or warm up the local OmniVoice TTS service."))
	static bool StartLocalTTS(const UObject* WorldContextObject, FString& ErrorMessage);

	UFUNCTION(BlueprintCallable, Category = "LocalTTS|Service", meta = (WorldContext = "WorldContextObject", DisplayName = "Stop Local TTS", ToolTip = "Stop the local TTS service process started by the plugin."))
	static void StopLocalTTS(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "LocalTTS|Playback", meta = (WorldContext = "WorldContextObject", DisplayName = "Stop Local TTS Speaking", ToolTip = "Stop the current LocalTTS audio playback."))
	static void StopSpeaking(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "LocalTTS|Service", meta = (WorldContext = "WorldContextObject", DisplayName = "Is Local TTS Ready", ToolTip = "Return true after the local TTS service health check reports ready."))
	static bool IsLocalTTSReady(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "LocalTTS|Service", meta = (WorldContext = "WorldContextObject", DisplayName = "Get Local TTS Service State", ToolTip = "Get the current LocalTTS service state."))
	static ELocalTTSServiceState GetLocalTTSServiceState(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "LocalTTS|Service", meta = (WorldContext = "WorldContextObject", DisplayName = "Get Local TTS Service State Text", ToolTip = "Get the current LocalTTS service state as readable text."))
	static FString GetLocalTTSServiceStateText(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "LocalTTS|Playback", meta = (WorldContext = "WorldContextObject", DisplayName = "Is Local TTS Speaking", ToolTip = "Return true while LocalTTS audio is currently playing."))
	static bool IsSpeaking(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "LocalTTS|Service", meta = (WorldContext = "WorldContextObject", DisplayName = "Is Local TTS Busy", ToolTip = "Return true while LocalTTS is generating or playing speech."))
	static bool IsLocalTTSBusy(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "LocalTTS|Debug", meta = (WorldContext = "WorldContextObject", DisplayName = "Get Last Local TTS Health", ToolTip = "Get the latest cached health response from the local TTS service."))
	static FLocalTTSHealthResponse GetLastLocalTTSHealth(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "LocalTTS|Debug", meta = (WorldContext = "WorldContextObject", DisplayName = "Get Last Local TTS Health Error", ToolTip = "Get the latest health check error message."))
	static FString GetLastLocalTTSError(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "LocalTTS|Debug", meta = (WorldContext = "WorldContextObject", DisplayName = "Get Last Local TTS Health Error Code", ToolTip = "Get the latest health check error code."))
	static ELocalTTSErrorCode GetLastLocalTTSHealthErrorCode(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "LocalTTS|Debug", meta = (WorldContext = "WorldContextObject", DisplayName = "Get Last Local TTS Speech Result", ToolTip = "Get the latest cached TTS response."))
	static FLocalTTSTTSResponse GetLastLocalTTSSpeechResult(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "LocalTTS|Debug", meta = (WorldContext = "WorldContextObject", DisplayName = "Get Last Local TTS WAV Path", ToolTip = "Get the wav path from the latest successful TTS response."))
	static FString GetLastLocalTTSWavPath(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "LocalTTS|Debug", meta = (WorldContext = "WorldContextObject", DisplayName = "Get Last Local TTS Speech Error", ToolTip = "Get the latest TTS request or playback error message."))
	static FString GetLastLocalTTSSpeechError(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "LocalTTS|Debug", meta = (WorldContext = "WorldContextObject", DisplayName = "Get Last Local TTS Speech Error Code", ToolTip = "Get the latest TTS request or playback error code."))
	static ELocalTTSErrorCode GetLastLocalTTSSpeechErrorCode(const UObject* WorldContextObject);

private:
	static class ULocalTTSSubsystem* ResolveSubsystem(const UObject* WorldContextObject);
};
