// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "FLocalTTSSpeakRequest.h"
#include "FLocalTTSLongTextRequest.generated.h"

// 长文本任务的输入结构，负责保存原始长文本、通用 TTS 模板和分段策略。
USTRUCT(BlueprintType)
struct LOCALTTS_API FLocalTTSLongTextRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LocalTTS|LongText", meta = (DisplayName = "原始长文本", ToolTip = "需要拆分并顺序生成/播放的完整文本内容。正式长文本链路请填写这里，而不是直接依赖模板请求里的 Text。"))
	FString SourceText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LocalTTS|LongText", meta = (DisplayName = "模板请求", ToolTip = "长文本队列会复用这份请求模板，并把每个文本段写入 Text 后逐段发送给 LocalTTS。"))
	FLocalTTSSpeakRequest SpeakRequestTemplate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LocalTTS|LongText|Split", meta = (DisplayName = "每段最大字符数", ClampMin = "8", ToolTip = "单个文本段允许的最大字符数。超过这个长度时会优先尝试按标点拆分，必要时再硬切。"))
	int32 MaxCharactersPerSegment = 80;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LocalTTS|LongText|Split", meta = (DisplayName = "每段最小合并字符数", ClampMin = "0", ToolTip = "当某个文本段过短时，队列会尝试和相邻文本段合并。0 表示不做最小段落合并。"))
	int32 MinCharactersPerSegment = 12;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LocalTTS|LongText|Split", meta = (DisplayName = "是否按换行拆分", ToolTip = "默认开启。开启后，换行会被当作优先分段边界。"))
	bool bSplitOnNewLine = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LocalTTS|LongText|Split", meta = (DisplayName = "是否自动套用推荐时长", ToolTip = "当模板请求里的 Duration 小于等于 0 时，是否自动把每段估算出的推荐时长写回请求。"))
	bool bUseRecommendedDuration = false;
};
