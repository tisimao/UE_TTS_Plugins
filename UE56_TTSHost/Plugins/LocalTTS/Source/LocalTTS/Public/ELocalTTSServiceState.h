// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ELocalTTSServiceState.generated.h"

UENUM(BlueprintType)
enum class ELocalTTSServiceState : uint8
{
	Stopped UMETA(DisplayName = "已停止", ToolTip = "本地 TTS 服务未运行。"),
	Starting UMETA(DisplayName = "启动中", ToolTip = "已经请求启动本地 TTS 服务，正在等待进程启动和模型加载。"),
	Ready UMETA(DisplayName = "已就绪", ToolTip = "本地 TTS 服务可访问，OmniVoice 模型已经 ready。"),
	Busy UMETA(DisplayName = "忙碌中", ToolTip = "本地 TTS 正在生成语音或播放语音。"),
	Error UMETA(DisplayName = "错误", ToolTip = "本地 TTS 服务或最近一次请求处于错误状态。")
};
