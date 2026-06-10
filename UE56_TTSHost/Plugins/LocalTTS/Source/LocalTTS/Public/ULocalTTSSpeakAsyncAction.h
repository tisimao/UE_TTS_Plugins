// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"

#include "ELocalTTSSpeakAsyncState.h"
#include "FLocalTTSSpeakRequest.h"
#include "FLocalTTSSpeechEvent.h"
#include "FLocalTTSTTSResponse.h"
#include "ULocalTTSSpeakAsyncAction.generated.h"

class AActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLocalTTSSpeakStartedDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLocalTTSSpeakSuccessDelegate, const FLocalTTSTTSResponse&, Response);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLocalTTSAudioReadyDelegate, const FLocalTTSTTSResponse&, Response);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLocalTTSSpeechEventReadyDelegate, const FLocalTTSSpeechEvent&, SpeechEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLocalTTSSpeakFinishedDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLocalTTSSpeakFailureDelegate, const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLocalTTSSpeakStateChangedDelegate, ELocalTTSSpeakAsyncState, State, const FString&, DetailMessage);

// 单次语音请求的蓝图异步节点，负责把服务启动、生成、音频就绪和播放完成统一封装成一条流程。
UCLASS()
class LOCALTTS_API ULocalTTSSpeakAsyncAction : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "LocalTTS|Speech", meta = (DisplayName = "开始生成", ToolTip = "请求已经进入 LocalTTS 语音流程。适合在这里把 UI 切到“思考中”或“生成中”。"))
	FLocalTTSSpeakStartedDelegate OnStarted;

	UPROPERTY(BlueprintAssignable, Category = "LocalTTS|Speech", meta = (DisplayName = "生成成功", ToolTip = "服务端已经成功返回本次 TTS 结果。这里表示 wav 已生成成功，但不一定已经开始播放。"))
	FLocalTTSSpeakSuccessDelegate OnSucceeded;

	UPROPERTY(BlueprintAssignable, Category = "LocalTTS|Speech", meta = (DisplayName = "音频已就绪", ToolTip = "音频已经准备完成。自动播放模式下，这通常是切 UI 到“播放中”的最佳时机；仅生成模式下，这表示 wav 已可供自定义系统使用。"))
	FLocalTTSAudioReadyDelegate OnAudioReady;

	UPROPERTY(BlueprintAssignable, Category = "LocalTTS|Speech", meta = (DisplayName = "语音事件已就绪", ToolTip = "语音事件结构已经生成，可用于数字人、口型、字幕或自定义时间线系统。"))
	FLocalTTSSpeechEventReadyDelegate OnSpeechEventReady;

	UPROPERTY(BlueprintAssignable, Category = "LocalTTS|Speech", meta = (DisplayName = "播放结束", ToolTip = "自动播放模式下，当前语音播放已经结束。可在这里恢复按钮、隐藏播放中状态。"))
	FLocalTTSSpeakFinishedDelegate OnFinished;

	UPROPERTY(BlueprintAssignable, Category = "LocalTTS|Speech", meta = (DisplayName = "发生错误", ToolTip = "语音流程任意阶段失败时触发，包括服务不可用、生成失败、wav 加载失败、播放失败等。"))
	FLocalTTSSpeakFailureDelegate OnError;

	UPROPERTY(BlueprintAssignable, Category = "LocalTTS|Speech", meta = (DisplayName = "状态变化", ToolTip = "统一的语音流程状态回调。正式 UI 推荐优先监听它来更新“思考中 / 生成中 / 播放中 / 失败”等状态。"))
	FLocalTTSSpeakStateChangedDelegate OnStateChanged;

	UFUNCTION(
		BlueprintCallable,
		meta = (
			BlueprintInternalUseOnly = "true",
			WorldContext = "WorldContextObject",
			DisplayName = "生成并播放 Local TTS",
			ToolTip = "通过本地 OmniVoice 服务异步生成语音，并在 Unreal 中以 2D 音频播放。正式 UI 推荐监听 OnStarted、OnAudioReady、OnFinished、OnError。"
		),
		Category = "LocalTTS|Speech")
	static ULocalTTSSpeakAsyncAction* SpeakTextAsync(UObject* WorldContextObject, const FLocalTTSSpeakRequest& SpeakRequest);

	UFUNCTION(
		BlueprintCallable,
		meta = (
			BlueprintInternalUseOnly = "true",
			WorldContext = "WorldContextObject",
			DisplayName = "生成并在 Actor 位置播放 Local TTS",
			ToolTip = "通过本地 OmniVoice 服务异步生成语音，并在目标 Actor 位置以 3D 音频播放。适合场景内角色、物体或数字人发声。"
		),
		Category = "LocalTTS|Speech")
	static ULocalTTSSpeakAsyncAction* SpeakTextAtActorAsync(UObject* WorldContextObject, const FLocalTTSSpeakRequest& SpeakRequest, AActor* PlaybackActor);

	UFUNCTION(
		BlueprintCallable,
		meta = (
			BlueprintInternalUseOnly = "true",
			WorldContext = "WorldContextObject",
			DisplayName = "仅生成 Local TTS",
			ToolTip = "通过本地 OmniVoice 服务异步生成语音，但不自动播放。适合数字人、自定义播放器、字幕或 wav 后处理链路。"
		),
		Category = "LocalTTS|Speech")
	static ULocalTTSSpeakAsyncAction* GenerateSpeechAsync(UObject* WorldContextObject, const FLocalTTSSpeakRequest& SpeakRequest);

	virtual void Activate() override;

private:
	void TrySendRequest();
	void BeginHealthPolling();
	void PollServiceHealth();
	void HandleSpeechResponse(const FLocalTTSTTSResponse& Response);
	void HandlePlaybackFinished();
	void BroadcastStateChanged(ELocalTTSSpeakAsyncState NewState, const FString& DetailMessage);
	void FinishWithFailure(const FString& ErrorMessage);
	void FinishWithSuccess(const FLocalTTSTTSResponse& Response);

	TWeakObjectPtr<UObject> WorldContextObject;
	TWeakObjectPtr<AActor> PlaybackActor;
	FLocalTTSSpeakRequest SpeakRequest;
	TWeakObjectPtr<class ULocalTTSSubsystem> Subsystem;
	FTimerHandle HealthPollTimerHandle;
	int32 RemainingHealthPollCount = 0;
	bool bAutoPlay = true;
	bool bUseActorPlayback = false;
};
