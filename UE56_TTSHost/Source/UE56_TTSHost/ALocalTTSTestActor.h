// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "FLocalTTSHealthResponse.h"
#include "FLocalTTSSpeakRequest.h"
#include "FLocalTTSSpeechEvent.h"
#include "FLocalTTSTTSResponse.h"
#include "ALocalTTSTestActor.generated.h"

UCLASS(
	BlueprintType,
	Blueprintable,
	meta = (
		DisplayName = "LocalTTS 示例 Actor",
		ToolTip = "仅用于在 PIE 中验证 LocalTTS 的示例 Actor。正式玩法蓝图请直接调用官方 LocalTTS 节点。"
	))
class UE56_TTSHOST_API ALocalTTSTestActor : public AActor
{
	GENERATED_BODY()

public:
	ALocalTTSTestActor();
	virtual void BeginPlay() override;

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "LocalTTS 示例", meta = (DisplayName = "检查服务健康", ToolTip = "示例操作：检查本地 TTS 服务健康状态，并把结果写入“最近健康摘要”。"))
	void CheckLocalTTSHealth();

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "LocalTTS 示例", meta = (DisplayName = "启动服务", ToolTip = "示例操作：启动本地 OmniVoice TTS 服务。首次启动时可能需要等待模型加载。"))
	void StartLocalTTSService();

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "LocalTTS 示例", meta = (DisplayName = "播放示例语音", ToolTip = "示例操作：生成语音并在 Unreal 中播放。仅用于验证端到端示例流程。"))
	void SpeakLocalTTSTest();

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "LocalTTS 示例", meta = (DisplayName = "仅生成示例 WAV", ToolTip = "示例操作：只生成 WAV，不自动播放。适合检查数字人或自定义播放集成点。"))
	void GenerateLocalTTSTest();

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "LocalTTS 示例", meta = (DisplayName = "停止播放", ToolTip = "示例操作：停止当前正在播放的 LocalTTS 音频。"))
	void StopLocalTTSSpeaking();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LocalTTS 示例", meta = (DisplayName = "示例朗读请求", ToolTip = "示例 Actor 使用的默认请求。正式蓝图请使用官方 LocalTTS 节点自行构建请求。"))
	FLocalTTSSpeakRequest SpeakRequest;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LocalTTS 示例|自动运行", meta = (DisplayName = "BeginPlay 时检查健康", ToolTip = "默认开启。PIE 开始时自动调用 /health。"))
	bool bCheckHealthOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LocalTTS 示例|自动运行", meta = (DisplayName = "BeginPlay 时启动服务", ToolTip = "默认开启。PIE 开始时自动尝试启动本地 TTS 服务。"))
	bool bStartServiceOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LocalTTS 示例|自动运行", meta = (DisplayName = "BeginPlay 时播放示例", ToolTip = "默认开启。等待“自动播放延迟秒数”后自动执行示例朗读。手动测试时可关闭。"))
	bool bSpeakOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LocalTTS 示例|自动运行", meta = (DisplayName = "自动播放延迟秒数", ClampMin = "0.1", ToolTip = "默认 3 秒。给本地服务和模型预留加载时间，再执行示例播放。"))
	float AutoSpeakDelaySeconds = 3.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LocalTTS 示例|结果", meta = (DisplayName = "最近健康响应", ToolTip = "该示例 Actor 最近一次捕获到的 /health 响应。"))
	FLocalTTSHealthResponse LastHealthResponse;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LocalTTS 示例|结果", meta = (DisplayName = "最近语音响应", ToolTip = "该示例 Actor 最近一次捕获到的 /tts 响应。"))
	FLocalTTSTTSResponse LastTTSResponse;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LocalTTS 示例|结果", meta = (DisplayName = "最近语音事件", ToolTip = "示例 Actor 暴露出的语音事件数据，可用于数字人集成测试。"))
	FLocalTTSSpeechEvent LastSpeechEvent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LocalTTS 示例|结果", meta = (DisplayName = "最近错误提示", ToolTip = "示例 Actor 最近一次记录的可读错误信息。示例流程失败时优先看这里。"))
	FString LastErrorMessage;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LocalTTS 示例|结果", meta = (DisplayName = "最近健康摘要", ToolTip = "最近一次健康检查的简要摘要，方便在 Details 面板快速查看。"))
	FString LastHealthSummary;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LocalTTS 示例|结果", meta = (DisplayName = "最近语音摘要", ToolTip = "最近一次语音生成结果的简要摘要，方便在 Details 面板快速查看。"))
	FString LastTTSSummary;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LocalTTS 示例|结果", meta = (DisplayName = "最近语音事件摘要", ToolTip = "最近一次由示例 Actor 生成的语音事件简要摘要。"))
	FString LastSpeechEventSummary;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LocalTTS 示例|结果", meta = (DisplayName = "最近生成的 WAV 路径", ToolTip = "示例流程最近一次生成的 WAV 绝对路径。"))
	FString LastGeneratedWavPath;

private:
	class ULocalTTSSubsystem* ResolveSubsystem() const;
	bool HasPlayableGeneratedAudio() const;
	void PlayCachedSpeech();
	void BeginSpeakFlow();
	void BeginGenerateFlow();
	void ExecuteDeferredSpeak();
	void ExecuteSpeakRequest();
	void ExecuteGenerateRequest();
	void BeginHealthPolling();
	void PollServiceHealth();
	void BeginHealthPollingForGenerate();
	void PollServiceHealthForGenerate();
	void StoreError(const FString& ErrorMessage);

	FTimerHandle HealthPollTimerHandle;
	int32 RemainingHealthPollCount = 0;
	bool bGenerateOnlyRequestInFlight = false;
	bool bPlayCachedSpeechAfterGenerate = false;
};
