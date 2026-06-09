// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FLocalTTSSpeakRequest.generated.h"

USTRUCT(BlueprintType)
struct LOCALTTS_API FLocalTTSSpeakRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LocalTTS|Request", meta = (DisplayName = "Text", ToolTip = "Text to synthesize."))
	FString Text;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LocalTTS|Request", meta = (DisplayName = "Mode", ToolTip = "OmniVoice mode. Supported values: auto, design, clone."))
	FString Mode = TEXT("auto");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LocalTTS|Request", meta = (DisplayName = "Language ID", ToolTip = "Language hint for OmniVoice, for example zh or en."))
	FString LanguageId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LocalTTS|Request|Reference", meta = (DisplayName = "Reference Audio Path", ToolTip = "Reference wav path. Required by clone mode."))
	FString ReferenceAudioPath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LocalTTS|Request|Reference", meta = (DisplayName = "Reference Text", ToolTip = "Text spoken in the reference audio, when available."))
	FString ReferenceText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LocalTTS|Request|Design", meta = (DisplayName = "Instruct", ToolTip = "Voice design instruction. Required by design mode."))
	FString Instruct;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LocalTTS|Request|Tuning", meta = (DisplayName = "Duration", ClampMin = "0.0", ToolTip = "Optional target duration in seconds. Use 0 to let the model decide."))
	float Duration = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LocalTTS|Request|Tuning", meta = (DisplayName = "Speed", ClampMin = "0.1", ClampMax = "3.0", ToolTip = "Speech speed multiplier. 1.0 is normal speed."))
	float Speed = 1.0f;
};
