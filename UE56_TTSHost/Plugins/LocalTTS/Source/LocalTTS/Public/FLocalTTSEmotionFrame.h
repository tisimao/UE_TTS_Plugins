// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FLocalTTSEmotionFrame.generated.h"

USTRUCT(BlueprintType)
struct LOCALTTS_API FLocalTTSEmotionFrame
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LocalTTS|DigitalHuman|Emotion", meta = (DisplayName = "时间秒", ToolTip = "从语音开始算起的时间点，单位为秒。用于表示某个表情帧出现的时间。"))
	float TimeSeconds = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LocalTTS|DigitalHuman|Emotion", meta = (DisplayName = "表情名称", ToolTip = "表情或情绪名称，例如 Neutral、Happy、Sad、Angry，或项目自定义表情 ID。"))
	FString EmotionName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LocalTTS|DigitalHuman|Emotion", meta = (DisplayName = "强度", ClampMin = "0.0", ClampMax = "1.0", ToolTip = "该表情帧的混合强度，范围 0 到 1。默认 1 表示完整应用该表情。"))
	float Intensity = 1.0f;
};
