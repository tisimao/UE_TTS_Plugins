// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FLocalTTSVisemeFrame.generated.h"

USTRUCT(BlueprintType)
struct LOCALTTS_API FLocalTTSVisemeFrame
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LocalTTS|DigitalHuman|Viseme", meta = (DisplayName = "时间秒", ToolTip = "从语音开始算起的时间点，单位为秒。用于表示某个口型帧出现的时间。"))
	float TimeSeconds = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LocalTTS|DigitalHuman|Viseme", meta = (DisplayName = "口型名称", ToolTip = "口型名称，例如 A、E、O、FV、MBP，或项目中约定的 MetaHuman 兼容口型 ID。"))
	FString VisemeName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LocalTTS|DigitalHuman|Viseme", meta = (DisplayName = "权重", ClampMin = "0.0", ClampMax = "1.0", ToolTip = "该口型帧的混合权重，范围 0 到 1。默认 1 表示完整应用该口型。"))
	float Weight = 1.0f;
};
