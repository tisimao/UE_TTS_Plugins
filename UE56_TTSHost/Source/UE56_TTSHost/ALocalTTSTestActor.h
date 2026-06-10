// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "FLocalTTSHealthResponse.h"
#include "FLocalTTSSpeakRequest.h"
#include "FLocalTTSSpeechEvent.h"
#include "FLocalTTSTTSResponse.h"
#include "ALocalTTSTestActor.generated.h"

UCLASS(BlueprintType, Blueprintable)
class UE56_TTSHOST_API ALocalTTSTestActor : public AActor
{
	GENERATED_BODY()

public:
	ALocalTTSTestActor();
	virtual void BeginPlay() override;

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "LocalTTS Test", meta = (DisplayName = "Health", ToolTip = "检查本地 TTS 服务健康状态，结果写入 LastHealthSummary。"))
	void CheckLocalTTSHealth();

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "LocalTTS Test", meta = (DisplayName = "Start", ToolTip = "启动本地 OmniVoice TTS 服务。首次启动需要等待模型加载完成。"))
	void StartLocalTTSService();

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "LocalTTS Test", meta = (DisplayName = "Speak", ToolTip = "生成语音并在 UE 内播放。用于验证端到端发声链路。"))
	void SpeakLocalTTSTest();

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "LocalTTS Test", meta = (DisplayName = "Gen", ToolTip = "只生成 wav，不自动播放。用于验证数字人或自定义播放链路入口。"))
	void GenerateLocalTTSTest();

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "LocalTTS Test", meta = (DisplayName = "Stop", ToolTip = "停止当前 LocalTTS 播放。"))
	void StopLocalTTSSpeaking();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LocalTTS Test", meta = (DisplayName = "测试语音请求", ToolTip = "测试 Actor 使用的默认请求。默认 Text 为中文测试句，Mode 为 auto，LanguageId 为 zh，Speed 为 1.0。"))
	FLocalTTSSpeakRequest SpeakRequest;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LocalTTS Test|Auto Run", meta = (DisplayName = "开始时检查健康", ToolTip = "默认 true。PIE BeginPlay 时自动请求 /health。"))
	bool bCheckHealthOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LocalTTS Test|Auto Run", meta = (DisplayName = "开始时启动服务", ToolTip = "默认 true。PIE BeginPlay 时自动尝试启动本地 TTS 服务。"))
	bool bStartServiceOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LocalTTS Test|Auto Run", meta = (DisplayName = "开始时自动朗读", ToolTip = "默认 true。PIE BeginPlay 后等待 AutoSpeakDelaySeconds 秒自动执行 Speak。手动测试时可以关闭。"))
	bool bSpeakOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LocalTTS Test|Auto Run", meta = (DisplayName = "自动朗读延迟秒数", ClampMin = "0.1", ToolTip = "默认 3 秒。给本地服务和模型加载留出时间，然后自动执行 Speak。"))
	float AutoSpeakDelaySeconds = 3.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LocalTTS Test|Result", meta = (DisplayName = "最近健康响应", ToolTip = "最近一次 Health、Speak 或 Gen 触发的 /health 结果。服务 ready 时通常会看到 ok=true、status=ready、model=OmniVoice。"))
	FLocalTTSHealthResponse LastHealthResponse;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LocalTTS Test|Result", meta = (DisplayName = "最近语音响应", ToolTip = "最近一次 Speak 或 Gen 的 /tts 结果。成功后会包含 request_id、wav 路径、采样率和生成耗时。"))
	FLocalTTSTTSResponse LastTTSResponse;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LocalTTS Test|Result", meta = (DisplayName = "最近语音事件", ToolTip = "面向数字人联动的语音事件结构。当前会填充文本、模式、wav 路径和时长信息；口型帧和表情帧为后续扩展预留。"))
	FLocalTTSSpeechEvent LastSpeechEvent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LocalTTS Test|Result", meta = (DisplayName = "最近错误信息", ToolTip = "最近一次失败时的可读错误。参数错误、服务未 ready、HTTP 失败、OmniVoice 推理失败都会优先看这里。"))
	FString LastErrorMessage;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LocalTTS Test|Result", meta = (DisplayName = "健康摘要", ToolTip = "健康检查的短文本摘要，便于在 Details 面板快速确认 ok、status 和 model。"))
	FString LastHealthSummary;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LocalTTS Test|Result", meta = (DisplayName = "语音摘要", ToolTip = "语音生成结果的短文本摘要，便于快速查看 ok、request_id、wav 路径和生成耗时。"))
	FString LastTTSSummary;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LocalTTS Test|Result", meta = (DisplayName = "语音事件摘要", ToolTip = "数字人语音事件的短文本摘要。后续接入口型或表情系统时可用它确认事件是否生成。"))
	FString LastSpeechEventSummary;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LocalTTS Test|Result", meta = (DisplayName = "最近生成 WAV 路径", ToolTip = "最近一次成功生成的 wav 文件绝对路径。可以用于手动播放确认，也可以交给口型分析或数字人驱动模块。"))
	FString LastGeneratedWavPath;

private:
	class ULocalTTSSubsystem* ResolveSubsystem() const;
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
};
