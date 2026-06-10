// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ELocalTTSLongTextQueueState.generated.h"

// 长文本队列的统一状态枚举，供蓝图 UI、字幕系统和数字人联动读取当前处理阶段。
UENUM(BlueprintType)
enum class ELocalTTSLongTextQueueState : uint8
{
	Idle UMETA(DisplayName = "空闲", ToolTip = "当前没有长文本任务在执行。"),
	Segmenting UMETA(DisplayName = "分段中", ToolTip = "正在把长文本拆分为多个文本段。"),
	Generating UMETA(DisplayName = "生成中", ToolTip = "正在顺序请求 LocalTTS 生成某一段音频。"),
	Playing UMETA(DisplayName = "播放中", ToolTip = "当前段音频已经开始播放。"),
	Paused UMETA(DisplayName = "已暂停", ToolTip = "长文本队列已暂停。播放中暂停会暂停当前音频，生成中暂停会在当前段完成后停在段落边界。"),
	Stopped UMETA(DisplayName = "已停止", ToolTip = "长文本队列已被手动停止。"),
	Finished UMETA(DisplayName = "已完成", ToolTip = "所有文本段已经顺序处理完成。"),
	Error UMETA(DisplayName = "错误", ToolTip = "长文本队列在某一段处理过程中发生错误。")
};
