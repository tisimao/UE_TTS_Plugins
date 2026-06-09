// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "FLocalTTSEmotionFrame.h"
#include "FLocalTTSSpeakRequest.h"
#include "FLocalTTSTTSResponse.h"
#include "FLocalTTSVisemeFrame.h"
#include "FLocalTTSSpeechEvent.generated.h"

USTRUCT(BlueprintType)
struct LOCALTTS_API FLocalTTSSpeechEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS|SpeechEvent", meta = (DisplayName = "OK", ToolTip = "Whether the speech event was generated from a successful TTS response."))
	bool bOk = false;

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS|SpeechEvent", meta = (DisplayName = "Request ID", ToolTip = "Service-side request id. Use this to match UE logs with service logs."))
	FString RequestId;

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS|SpeechEvent", meta = (DisplayName = "Text", ToolTip = "Text used to generate this speech event."))
	FString Text;

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS|SpeechEvent", meta = (DisplayName = "Mode", ToolTip = "OmniVoice mode used by this speech event."))
	FString Mode;

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS|SpeechEvent", meta = (DisplayName = "Language ID", ToolTip = "Language hint used by this speech event."))
	FString LanguageId;

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS|SpeechEvent", meta = (DisplayName = "WAV Path", ToolTip = "Generated wav file path on local disk. Digital human systems can analyze this file for lip sync."))
	FString WavPath;

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS|SpeechEvent", meta = (DisplayName = "Sample Rate", ToolTip = "Generated audio sample rate."))
	int32 SampleRate = 0;

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS|SpeechEvent", meta = (DisplayName = "Duration MS", ToolTip = "Service-side synthesis duration in milliseconds."))
	int32 DurationMs = 0;

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS|SpeechEvent", meta = (DisplayName = "Duration Seconds", ToolTip = "Speech duration in seconds when known."))
	float DurationSeconds = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS|SpeechEvent|DigitalHuman", meta = (DisplayName = "Viseme Frames", ToolTip = "Reserved lip sync frames. Current OmniVoice path does not fill these yet."))
	TArray<FLocalTTSVisemeFrame> VisemeFrames;

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS|SpeechEvent|DigitalHuman", meta = (DisplayName = "Emotion Frames", ToolTip = "Reserved expression frames. Current OmniVoice path does not fill these yet."))
	TArray<FLocalTTSEmotionFrame> EmotionFrames;

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS|SpeechEvent|Error", meta = (DisplayName = "Error Code", ToolTip = "Service-side error code when the request failed."))
	FString ErrorCode;

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS|SpeechEvent|Error", meta = (DisplayName = "Error Message", ToolTip = "Readable error message when the request failed."))
	FString ErrorMessage;

	static FLocalTTSSpeechEvent FromRequestAndResponse(
		const FLocalTTSSpeakRequest& SpeakRequest,
		const FLocalTTSTTSResponse& TTSResponse)
	{
		FLocalTTSSpeechEvent SpeechEvent;
		SpeechEvent.bOk = TTSResponse.bOk;
		SpeechEvent.RequestId = TTSResponse.RequestId;
		SpeechEvent.Text = SpeakRequest.Text;
		SpeechEvent.Mode = TTSResponse.Mode.IsEmpty() ? SpeakRequest.Mode : TTSResponse.Mode;
		SpeechEvent.LanguageId = SpeakRequest.LanguageId;
		SpeechEvent.WavPath = TTSResponse.WavPath;
		SpeechEvent.SampleRate = TTSResponse.SampleRate;
		SpeechEvent.DurationMs = TTSResponse.DurationMs;
		SpeechEvent.DurationSeconds = TTSResponse.DurationMs > 0 ? static_cast<float>(TTSResponse.DurationMs) / 1000.0f : SpeakRequest.Duration;
		SpeechEvent.ErrorCode = TTSResponse.ErrorCode;
		SpeechEvent.ErrorMessage = TTSResponse.ErrorMessage;
		return SpeechEvent;
	}
};
