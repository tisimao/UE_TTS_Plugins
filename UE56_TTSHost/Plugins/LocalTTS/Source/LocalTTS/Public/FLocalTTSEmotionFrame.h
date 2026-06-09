// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FLocalTTSEmotionFrame.generated.h"

USTRUCT(BlueprintType)
struct LOCALTTS_API FLocalTTSEmotionFrame
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LocalTTS|DigitalHuman|Emotion", meta = (DisplayName = "Time Seconds", ToolTip = "Time offset in seconds from the beginning of the generated speech."))
	float TimeSeconds = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LocalTTS|DigitalHuman|Emotion", meta = (DisplayName = "Emotion Name", ToolTip = "Expression or emotion name, for example Neutral, Happy, Sad, Angry, or a project-specific expression id."))
	FString EmotionName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LocalTTS|DigitalHuman|Emotion", meta = (DisplayName = "Intensity", ClampMin = "0.0", ClampMax = "1.0", ToolTip = "Blend intensity for this expression frame."))
	float Intensity = 1.0f;
};
