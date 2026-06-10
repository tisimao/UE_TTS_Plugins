// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "FLocalTTSSpeechEvent.h"
#include "FLocalTTSTextSegment.h"
#include "FLocalTTSSegmentSpeechEvent.generated.h"

// 把“文本段信息”和“该段生成出的语音事件”组合在一起，方便字幕和数字人系统直接消费。
USTRUCT(BlueprintType)
struct LOCALTTS_API FLocalTTSSegmentSpeechEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS|LongText", meta = (DisplayName = "文本段", ToolTip = "当前处理的文本段信息。"))
	FLocalTTSTextSegment Segment;

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS|LongText", meta = (DisplayName = "语音事件", ToolTip = "当前文本段对应的语音事件结果。"))
	FLocalTTSSpeechEvent SpeechEvent;
};
