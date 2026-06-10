// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "ELocalTTSErrorCode.h"
#include "ELocalTTSLongTextQueueState.h"
#include "ELocalTTSSpeakAsyncState.h"
#include "ELocalTTSServiceState.h"
#include "FLocalTTSHealthResponse.h"
#include "FLocalTTSLongTextRequest.h"
#include "FLocalTTSSpeakRequest.h"
#include "FLocalTTSTTSResponse.h"
#include "ULocalTTSBlueprintLibrary.generated.h"

UCLASS()
class LOCALTTS_API ULocalTTSBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(
		BlueprintPure,
		Category = "LocalTTS|Request",
		meta = (
			DisplayName = "创建 Local TTS 自动模式请求",
			ToolTip = "构建一个 OmniVoice auto 模式请求。适合快速文本转语音测试，也适合作为默认请求构造入口。"
		))
	static FLocalTTSSpeakRequest MakeAutoSpeakRequest(const FString& Text, const FString& LanguageId, float Speed);

	UFUNCTION(
		BlueprintPure,
		Category = "LocalTTS|Request",
		meta = (
			DisplayName = "创建 Local TTS 音色设计请求",
			ToolTip = "构建一个 OmniVoice design 模式请求。需要填写受支持的 Instruct 标签，例如 'female, chinese accent'。"
		))
	static FLocalTTSSpeakRequest MakeDesignSpeakRequest(const FString& Text, const FString& Instruct, const FString& LanguageId, float Speed);

	UFUNCTION(
		BlueprintPure,
		Category = "LocalTTS|Request",
		meta = (
			DisplayName = "创建 Local TTS 音色克隆请求",
			ToolTip = "构建一个 OmniVoice clone 模式请求。ReferenceAudioPath 必须指向本机已存在的 WAV 文件。"
		))
	static FLocalTTSSpeakRequest MakeCloneSpeakRequest(const FString& Text, const FString& ReferenceAudioPath, const FString& ReferenceText, const FString& LanguageId, float Speed);

	UFUNCTION(
		BlueprintCallable,
		Category = "LocalTTS|Request",
		meta = (
			DisplayName = "校验 Local TTS 请求",
			ToolTip = "在发送到本地 OmniVoice 服务前，先检查请求参数是否合法。"
		))
	static bool ValidateSpeakRequest(const FLocalTTSSpeakRequest& SpeakRequest, FString& ErrorMessage);

	UFUNCTION(
		BlueprintCallable,
		Category = "LocalTTS|Service",
		meta = (
			WorldContext = "WorldContextObject",
			DisplayName = "启动 Local TTS",
			ToolTip = "启动或预热本地 OmniVoice 服务。正式发起语音请求前，建议等待健康状态进入“就绪”。"
		))
	static bool StartLocalTTS(const UObject* WorldContextObject, FString& ErrorMessage);

	UFUNCTION(
		BlueprintCallable,
		Category = "LocalTTS|Service",
		meta = (
			WorldContext = "WorldContextObject",
			DisplayName = "停止 Local TTS",
			ToolTip = "停止插件拉起的 LocalTTS 服务进程，并中断当前播放。"
		))
	static void StopLocalTTS(const UObject* WorldContextObject);

	UFUNCTION(
		BlueprintCallable,
		Category = "LocalTTS|Playback",
		meta = (
			WorldContext = "WorldContextObject",
			DisplayName = "停止 Local TTS 播放",
			ToolTip = "停止当前正在 Unreal 中播放的 LocalTTS 音频。"
		))
	static void StopSpeaking(const UObject* WorldContextObject);

	UFUNCTION(
		BlueprintCallable,
		Category = "LocalTTS|Playback",
		meta = (
			WorldContext = "WorldContextObject",
			DisplayName = "暂停 Local TTS 播放",
			ToolTip = "暂停当前正在 Unreal 中播放的 LocalTTS 音频。仅对正在播放的音频有效。"
		))
	static bool PauseSpeaking(const UObject* WorldContextObject, FString& ErrorMessage);

	UFUNCTION(
		BlueprintCallable,
		Category = "LocalTTS|Playback",
		meta = (
			WorldContext = "WorldContextObject",
			DisplayName = "恢复 Local TTS 播放",
			ToolTip = "恢复当前已暂停的 LocalTTS 音频。"
		))
	static bool ResumeSpeaking(const UObject* WorldContextObject, FString& ErrorMessage);

	UFUNCTION(
		BlueprintPure,
		Category = "LocalTTS|Service",
		meta = (
			WorldContext = "WorldContextObject",
			DisplayName = "Local TTS 是否就绪",
			ToolTip = "当最近一次 /health 响应显示本地服务状态为 Ready 时，返回 true。"
		))
	static bool IsLocalTTSReady(const UObject* WorldContextObject);

	UFUNCTION(
		BlueprintPure,
		Category = "LocalTTS|Service",
		meta = (
			WorldContext = "WorldContextObject",
			DisplayName = "获取 Local TTS 服务状态",
			ToolTip = "读取当前 LocalTTS 服务状态枚举值。"
		))
	static ELocalTTSServiceState GetLocalTTSServiceState(const UObject* WorldContextObject);

	UFUNCTION(
		BlueprintPure,
		Category = "LocalTTS|Service",
		meta = (
			WorldContext = "WorldContextObject",
			DisplayName = "获取 Local TTS 服务状态文本",
			ToolTip = "读取当前 LocalTTS 服务状态的可读文本，适合直接显示到 UI。"
		))
	static FString GetLocalTTSServiceStateText(const UObject* WorldContextObject);

	UFUNCTION(
		BlueprintPure,
		Category = "LocalTTS|Playback",
		meta = (
			WorldContext = "WorldContextObject",
			DisplayName = "Local TTS 是否正在播放",
			ToolTip = "当 LocalTTS 音频当前正在 Unreal 中播放时，返回 true。"
		))
	static bool IsSpeaking(const UObject* WorldContextObject);

	UFUNCTION(
		BlueprintPure,
		Category = "LocalTTS|Service",
		meta = (
			WorldContext = "WorldContextObject",
			DisplayName = "Local TTS 是否忙碌",
			ToolTip = "当语音请求仍在执行或音频还在播放时，返回 true。适合用于按钮节流。"
		))
	static bool IsLocalTTSBusy(const UObject* WorldContextObject);

	UFUNCTION(
		BlueprintPure,
		Category = "LocalTTS|UI",
		meta = (
			DisplayName = "获取语音异步状态文本",
			ToolTip = "把 LocalTTS 语音异步状态枚举转换成适合直接显示到 UI 的中文文本。"
		))
	static FString GetLocalTTSSpeakAsyncStateText(ELocalTTSSpeakAsyncState State);

	UFUNCTION(
		BlueprintPure,
		Category = "LocalTTS|UI",
		meta = (
			DisplayName = "语音异步状态是否应禁用按钮",
			ToolTip = "当状态处于等待服务、生成中或播放中时返回 true。适合用来禁用“播放”或“重新生成”按钮。"
		))
	static bool ShouldDisableSubmitForLocalTTSSpeakState(ELocalTTSSpeakAsyncState State);

	UFUNCTION(
		BlueprintPure,
		Category = "LocalTTS|UI",
		meta = (
			DisplayName = "语音异步状态是否已结束",
			ToolTip = "当状态已经结束或发生错误时返回 true。适合用来恢复按钮、隐藏加载提示。"
		))
	static bool IsTerminalLocalTTSSpeakState(ELocalTTSSpeakAsyncState State);

	UFUNCTION(
		BlueprintCallable,
		Category = "LocalTTS|LongText",
		meta = (
			WorldContext = "WorldContextObject",
			DisplayName = "创建 Local TTS 长文本队列",
			ToolTip = "创建一个可重复使用的 LocalTTS 长文本队列对象，用于分段生成、顺序播放和数字人联动。"
		))
	static class ULocalTTSLongTextQueue* CreateLocalTTSLongTextQueue(UObject* WorldContextObject);

	UFUNCTION(
		BlueprintPure,
		Category = "LocalTTS|LongText|UI",
		meta = (
			DisplayName = "获取长文本队列状态文本",
			ToolTip = "把 LocalTTS 长文本队列状态枚举转换成适合直接显示到 UI 的中文文本。"
		))
	static FString GetLocalTTSLongTextQueueStateText(ELocalTTSLongTextQueueState State);

	UFUNCTION(
		BlueprintPure,
		Category = "LocalTTS|LongText|UI",
		meta = (
			DisplayName = "长文本队列状态是否可开始",
			ToolTip = "当长文本队列处于空闲、已停止、已完成或错误状态时返回 true。适合控制“开始长文本播放/仅生成”按钮。"
		))
	static bool CanStartLocalTTSLongTextQueue(ELocalTTSLongTextQueueState State);

	UFUNCTION(
		BlueprintPure,
		Category = "LocalTTS|LongText|UI",
		meta = (
			DisplayName = "长文本队列状态是否可暂停",
			ToolTip = "当长文本队列处于分段中、生成中或播放中时返回 true。适合控制“暂停”按钮。"
		))
	static bool CanPauseLocalTTSLongTextQueue(ELocalTTSLongTextQueueState State);

	UFUNCTION(
		BlueprintPure,
		Category = "LocalTTS|LongText|UI",
		meta = (
			DisplayName = "长文本队列状态是否可继续",
			ToolTip = "当长文本队列处于已暂停状态时返回 true。适合控制“继续”按钮。"
		))
	static bool CanResumeLocalTTSLongTextQueue(ELocalTTSLongTextQueueState State);

	UFUNCTION(
		BlueprintPure,
		Category = "LocalTTS|LongText|UI",
		meta = (
			DisplayName = "长文本队列状态是否可跳到下一段",
			ToolTip = "当长文本队列处于生成中、播放中或已暂停状态时返回 true。适合控制“下一段”按钮。"
		))
	static bool CanSkipLocalTTSLongTextQueue(ELocalTTSLongTextQueueState State);

	UFUNCTION(
		BlueprintPure,
		Category = "LocalTTS|LongText|UI",
		meta = (
			DisplayName = "长文本队列状态是否可停止",
			ToolTip = "当长文本队列正在分段、生成、播放或暂停时返回 true。适合控制“停止”按钮。"
		))
	static bool CanStopLocalTTSLongTextQueue(ELocalTTSLongTextQueueState State);

	UFUNCTION(
		BlueprintPure,
		Category = "LocalTTS|LongText|UI",
		meta = (
			DisplayName = "长文本队列状态是否已结束",
			ToolTip = "当长文本队列已停止、已完成或发生错误时返回 true。适合恢复 UI 交互或隐藏加载提示。"
		))
	static bool IsTerminalLocalTTSLongTextQueueState(ELocalTTSLongTextQueueState State);

	UFUNCTION(
		BlueprintPure,
		Category = "LocalTTS|Debug",
		meta = (
			WorldContext = "WorldContextObject",
			DisplayName = "获取最近 Local TTS 健康响应",
			ToolTip = "读取最近一次缓存的 /health 响应，可用于调试或 UI 显示。"
		))
	static FLocalTTSHealthResponse GetLastLocalTTSHealth(const UObject* WorldContextObject);

	UFUNCTION(
		BlueprintPure,
		Category = "LocalTTS|Debug",
		meta = (
			WorldContext = "WorldContextObject",
			DisplayName = "获取最近 Local TTS 健康错误",
			ToolTip = "读取最近一次健康检查失败时的错误信息。"
		))
	static FString GetLastLocalTTSError(const UObject* WorldContextObject);

	UFUNCTION(
		BlueprintPure,
		Category = "LocalTTS|Debug",
		meta = (
			WorldContext = "WorldContextObject",
			DisplayName = "获取最近 Local TTS 健康错误码",
			ToolTip = "读取最近一次健康检查失败时的错误码。"
		))
	static ELocalTTSErrorCode GetLastLocalTTSHealthErrorCode(const UObject* WorldContextObject);

	UFUNCTION(
		BlueprintPure,
		Category = "LocalTTS|Debug",
		meta = (
			WorldContext = "WorldContextObject",
			DisplayName = "获取最近 Local TTS 语音结果",
			ToolTip = "读取最近一次成功的 /tts 响应。"
		))
	static FLocalTTSTTSResponse GetLastLocalTTSSpeechResult(const UObject* WorldContextObject);

	UFUNCTION(
		BlueprintPure,
		Category = "LocalTTS|Debug",
		meta = (
			WorldContext = "WorldContextObject",
			DisplayName = "获取最近 Local TTS WAV 路径",
			ToolTip = "读取最近一次成功语音请求生成的 WAV 路径。"
		))
	static FString GetLastLocalTTSWavPath(const UObject* WorldContextObject);

	UFUNCTION(
		BlueprintPure,
		Category = "LocalTTS|Debug",
		meta = (
			WorldContext = "WorldContextObject",
			DisplayName = "获取最近 Local TTS 语音错误",
			ToolTip = "读取最近一次语音生成、WAV 加载或播放失败时的错误信息。"
		))
	static FString GetLastLocalTTSSpeechError(const UObject* WorldContextObject);

	UFUNCTION(
		BlueprintPure,
		Category = "LocalTTS|Debug",
		meta = (
			WorldContext = "WorldContextObject",
			DisplayName = "获取最近 Local TTS 语音错误码",
			ToolTip = "读取最近一次语音生成、WAV 加载或播放失败时的错误码。"
		))
	static ELocalTTSErrorCode GetLastLocalTTSSpeechErrorCode(const UObject* WorldContextObject);

private:
	static class ULocalTTSSubsystem* ResolveSubsystem(const UObject* WorldContextObject);
};
