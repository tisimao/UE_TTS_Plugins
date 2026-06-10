// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ELocalTTSSpeakAsyncState.generated.h"

// 单次语音异步请求的统一状态枚举，正式 UI 推荐直接基于它切换“思考中/生成中/播放中”等提示。
UENUM(BlueprintType)
enum class ELocalTTSSpeakAsyncState : uint8
{
	Started = 0 UMETA(DisplayName = "请求开始", ToolTip = "语音请求已经进入 LocalTTS 流程。适合让 UI 进入“思考中”或“生成中”。"),
	WaitingForService = 1 UMETA(DisplayName = "等待服务就绪", ToolTip = "服务正在启动或模型仍在加载，尚未进入正式生成阶段。"),
	Generating = 2 UMETA(DisplayName = "生成中", ToolTip = "请求已经发送到本地 TTS 服务，正在等待服务端返回生成结果。"),
	AudioReady = 3 UMETA(DisplayName = "音频已就绪", ToolTip = "WAV 已经生成完成，并且已经可以交给播放、数字人或自定义系统使用。"),
	Playing = 4 UMETA(DisplayName = "播放中", ToolTip = "自动播放模式下，音频已经开始在 Unreal 中播放。"),
	Finished = 5 UMETA(DisplayName = "流程结束", ToolTip = "本次语音流程已经完成。自动播放模式表示播放结束，仅生成模式表示生成结束。"),
	Error = 6 UMETA(DisplayName = "发生错误", ToolTip = "语音流程在某个阶段失败。可结合错误消息显示 UI 提示。"),
	Idle = 255 UMETA(DisplayName = "空闲", ToolTip = "当前没有单句语音任务在执行。适合作为 UI 初始状态。")
};
