// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FLocalTTSTextSegment.generated.h"

// 表示长文本拆分后的单个文本段，后续队列会按这个结构逐段生成与播放。
USTRUCT(BlueprintType)
struct LOCALTTS_API FLocalTTSTextSegment
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS|LongText", meta = (DisplayName = "段序号", ToolTip = "当前文本段在整段长文本中的顺序编号，从 0 开始。"))
	int32 SegmentIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS|LongText", meta = (DisplayName = "段文本", ToolTip = "拆分后的单段文本内容。"))
	FString Text;

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS|LongText", meta = (DisplayName = "原文起始位置", ToolTip = "该文本段在原始长文本中的起始字符位置。"))
	int32 SourceStart = 0;

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS|LongText", meta = (DisplayName = "原文长度", ToolTip = "该文本段在原始长文本中的字符长度。"))
	int32 SourceLength = 0;

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS|LongText", meta = (DisplayName = "推荐目标时长", ToolTip = "根据文本长度估算的推荐时长，单位为秒。当前主要用于后续扩展数字人和节奏控制。"))
	float RecommendedDuration = 0.0f;
};
