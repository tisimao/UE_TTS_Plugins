// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"

#include "ELocalTTSSpeakAsyncState.h"
#include "FLocalTTSSpeechEvent.h"
#include "FLocalTTSTTSResponse.h"
#include "ULocalTTSPlayWavAsyncAction.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLocalTTSWavPlaybackReadyDelegate, const FLocalTTSTTSResponse&, Response);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLocalTTSWavPlaybackFinishedDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLocalTTSWavPlaybackFailureDelegate, const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLocalTTSWavPlaybackStateChangedDelegate, ELocalTTSSpeakAsyncState, State, const FString&, DetailMessage);

UCLASS()
class LOCALTTS_API ULocalTTSPlayWavAsyncAction : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "LocalTTS|Playback", meta = (DisplayName = "音频已就绪", ToolTip = "WAV 已经加载并开始进入播放流程。"))
	FLocalTTSWavPlaybackReadyDelegate OnAudioReady;

	UPROPERTY(BlueprintAssignable, Category = "LocalTTS|Playback", meta = (DisplayName = "播放结束", ToolTip = "当前 WAV 播放结束。"))
	FLocalTTSWavPlaybackFinishedDelegate OnFinished;

	UPROPERTY(BlueprintAssignable, Category = "LocalTTS|Playback", meta = (DisplayName = "发生错误", ToolTip = "WAV 路径无效、加载失败或播放失败时触发。"))
	FLocalTTSWavPlaybackFailureDelegate OnError;

	UPROPERTY(BlueprintAssignable, Category = "LocalTTS|Playback", meta = (DisplayName = "状态变化", ToolTip = "播放流程状态变化，可用于驱动 UI。"))
	FLocalTTSWavPlaybackStateChangedDelegate OnStateChanged;

	UFUNCTION(
		BlueprintCallable,
		meta = (
			BlueprintInternalUseOnly = "true",
			WorldContext = "WorldContextObject",
			DisplayName = "播放 Local TTS WAV",
			ToolTip = "播放指定 WAV 文件，不会请求 TTS 服务生成新语音。"
		),
		Category = "LocalTTS|Playback")
	static ULocalTTSPlayWavAsyncAction* PlayWavPathAsync(UObject* WorldContextObject, const FString& WavPath);

	UFUNCTION(
		BlueprintCallable,
		meta = (
			BlueprintInternalUseOnly = "true",
			WorldContext = "WorldContextObject",
			DisplayName = "播放最新 Local TTS WAV",
			ToolTip = "播放最近一次生成成功的 WAV，不会请求 TTS 服务生成新语音。"
		),
		Category = "LocalTTS|Playback")
	static ULocalTTSPlayWavAsyncAction* PlayLastWavAsync(UObject* WorldContextObject);

	UFUNCTION(
		BlueprintCallable,
		meta = (
			BlueprintInternalUseOnly = "true",
			WorldContext = "WorldContextObject",
			DisplayName = "播放 Local TTS 响应",
			ToolTip = "播放一个已有 TTS 响应中的 WAV，适合从历史列表里重播旧声音。"
		),
		Category = "LocalTTS|Playback")
	static ULocalTTSPlayWavAsyncAction* PlaySpeechResponseAsync(UObject* WorldContextObject, const FLocalTTSTTSResponse& Response);

	UFUNCTION(
		BlueprintCallable,
		meta = (
			BlueprintInternalUseOnly = "true",
			WorldContext = "WorldContextObject",
			DisplayName = "播放 Local TTS 语音事件",
			ToolTip = "播放一个已有语音事件中的 WAV，适合字幕、数字人或长文本段落重播。"
		),
		Category = "LocalTTS|Playback")
	static ULocalTTSPlayWavAsyncAction* PlaySpeechEventAsync(UObject* WorldContextObject, const FLocalTTSSpeechEvent& SpeechEvent);

	virtual void Activate() override;

private:
	enum class EPlaybackSource : uint8
	{
		WavPath,
		Last,
		Response
	};

	void FinishWithFailure(const FString& ErrorMessage);
	void BroadcastStateChanged(ELocalTTSSpeakAsyncState NewState, const FString& DetailMessage);

	TWeakObjectPtr<UObject> WorldContextObject;
	FString WavPath;
	FLocalTTSTTSResponse Response;
	EPlaybackSource PlaybackSource = EPlaybackSource::WavPath;
};
