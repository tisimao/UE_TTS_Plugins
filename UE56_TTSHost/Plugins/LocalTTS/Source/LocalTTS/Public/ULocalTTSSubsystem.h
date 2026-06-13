// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "ELocalTTSErrorCode.h"
#include "ELocalTTSServiceState.h"
#include "FLocalTTSHealthResponse.h"
#include "FLocalTTSRequestValidator.h"
#include "FLocalTTSServiceProcess.h"
#include "FLocalTTSSpeakRequest.h"
#include "FLocalTTSTTSResponse.h"
#include "FLocalTTSWavLoader.h"
#include "ULocalTTSSubsystem.generated.h"

class FLocalTTSHttpClient;
class ULocalTTSAudioPlayer;
class AActor;

UCLASS()
class LOCALTTS_API ULocalTTSSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	ULocalTTSSubsystem();
	virtual ~ULocalTTSSubsystem() override;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	void CheckHealth(
		TFunction<void(const FLocalTTSHealthResponse&)>&& OnSuccess,
		TFunction<void(const FString&)>&& OnFailure);

	bool StartService(FString& OutErrorMessage);
	void StopService();
	void StopSpeaking();
	bool PauseSpeaking(FString& OutErrorMessage);
	bool ResumeSpeaking(FString& OutErrorMessage);
	bool IsSpeaking() const;
	bool IsSpeakingPaused() const;
	bool IsTTSRequestInFlight() const;
	bool IsBusy() const;

	void SpeakText(
		const FLocalTTSSpeakRequest& SpeakRequest,
		TFunction<void(const FLocalTTSTTSResponse&)>&& OnSuccess,
		TFunction<void(const FString&)>&& OnFailure);

	void PlaySpeech(
		UObject* WorldContextObject,
		const FLocalTTSTTSResponse& TTSResponse,
		TFunction<void()>&& OnAudioReady,
		TFunction<void()>&& OnFinished,
		TFunction<void(const FString&)>&& OnFailure);

	void PlayWavPath(
		UObject* WorldContextObject,
		const FString& WavPath,
		TFunction<void(const FLocalTTSTTSResponse&)>&& OnAudioReady,
		TFunction<void()>&& OnFinished,
		TFunction<void(const FString&)>&& OnFailure);

	void PlayLastSpeech(
		UObject* WorldContextObject,
		TFunction<void(const FLocalTTSTTSResponse&)>&& OnAudioReady,
		TFunction<void()>&& OnFinished,
		TFunction<void(const FString&)>&& OnFailure);

	void PlaySpeechAtActor(
		UObject* WorldContextObject,
		const FLocalTTSTTSResponse& TTSResponse,
		AActor* PlaybackActor,
		TFunction<void()>&& OnAudioReady,
		TFunction<void()>&& OnFinished,
		TFunction<void(const FString&)>&& OnFailure);

	UFUNCTION(BlueprintPure, Category = "LocalTTS", meta = (DisplayName = "获取服务地址", ToolTip = "读取插件当前使用的本地 TTS 服务地址。默认来自项目设置： http://127.0.0.1:50021。"))
	FString GetServiceBaseUrl() const;

	UFUNCTION(BlueprintPure, Category = "LocalTTS", meta = (DisplayName = "服务是否就绪", ToolTip = "当最近一次 /health 返回 ok=true 且 status=ready 时为 true。首次加载 OmniVoice 模型时通常会先是 false。"))
	bool IsServiceReady() const;

	UFUNCTION(BlueprintPure, Category = "LocalTTS", meta = (DisplayName = "获取服务状态", ToolTip = "读取插件内部服务状态。默认初始值为已停止，启动后会在启动中、已就绪、忙碌中、错误之间切换。"))
	ELocalTTSServiceState GetServiceState() const;

	UFUNCTION(BlueprintPure, Category = "LocalTTS", meta = (DisplayName = "获取服务状态文本", ToolTip = "读取服务状态的可读文本，适合直接显示到调试 UI 或 Details 面板中。"))
	FString GetServiceStateText() const;

	UFUNCTION(BlueprintPure, Category = "LocalTTS", meta = (DisplayName = "获取最近健康响应", ToolTip = "读取最近一次成功解析的 /health 响应。默认是空响应；点击 Health、Speak 或 Gen 后会更新。"))
	FLocalTTSHealthResponse GetLastHealthResponse() const;

	UFUNCTION(BlueprintPure, Category = "LocalTTS", meta = (DisplayName = "获取最近健康错误", ToolTip = "读取最近一次健康检查失败时的错误信息。默认为空；服务未启动、端口不通或模型加载失败时会写入。"))
	FString GetLastHealthError() const;

	UFUNCTION(BlueprintPure, Category = "LocalTTS", meta = (DisplayName = "获取最近健康错误码", ToolTip = "读取最近一次健康检查失败时的错误码。默认无错误；可在蓝图中按错误类型分支处理。"))
	ELocalTTSErrorCode GetLastHealthErrorCode() const;

	UFUNCTION(BlueprintPure, Category = "LocalTTS", meta = (DisplayName = "获取最近语音响应", ToolTip = "读取最近一次 /tts 响应。成功时包含 request_id、wav 路径、采样率和生成耗时；默认是空响应。"))
	FLocalTTSTTSResponse GetLastTTSResponse() const;

	UFUNCTION(BlueprintPure, Category = "LocalTTS", meta = (DisplayName = "获取 Local TTS 语音历史", ToolTip = "读取最近生成成功的 TTS 响应列表，可用于播放旧 WAV。"))
	TArray<FLocalTTSTTSResponse> GetTTSResponseHistory() const;

	UFUNCTION(BlueprintPure, Category = "LocalTTS", meta = (DisplayName = "获取最近语音错误", ToolTip = "读取最近一次语音生成、WAV 加载或播放失败时的错误信息。默认为空。"))
	FString GetLastTTSError() const;

	UFUNCTION(BlueprintPure, Category = "LocalTTS", meta = (DisplayName = "获取最近语音错误码", ToolTip = "读取最近一次语音流程失败时的错误码。默认无错误；可用于 UI 提示或自动重试逻辑。"))
	ELocalTTSErrorCode GetLastTTSErrorCode() const;

private:
	void UpdateHealthState(const FLocalTTSHealthResponse& Response);
	void UpdateHealthFailure(const FString& ErrorMessage, ELocalTTSErrorCode ErrorCode = ELocalTTSErrorCode::InternalError);
	void UpdateTTSState(const FLocalTTSTTSResponse& Response);
	void UpdateTTSFailure(const FString& ErrorMessage, ELocalTTSErrorCode ErrorCode = ELocalTTSErrorCode::InternalError);
	void HandlePlaybackFinished();

	TSharedPtr<FLocalTTSHttpClient> HttpClient;
	TUniquePtr<FLocalTTSRequestValidator> RequestValidator;
	TUniquePtr<FLocalTTSServiceProcess> ServiceProcess;
	TUniquePtr<FLocalTTSWavLoader> WavLoader;

	UPROPERTY(Transient)
	TObjectPtr<ULocalTTSAudioPlayer> AudioPlayer;

	FLocalTTSHealthResponse LastHealthResponse;
	FLocalTTSTTSResponse LastTTSResponse;
	TArray<FLocalTTSTTSResponse> TTSResponseHistory;
	FString LastHealthError;
	FString LastTTSError;
	ELocalTTSErrorCode LastHealthErrorCode = ELocalTTSErrorCode::None;
	ELocalTTSErrorCode LastTTSErrorCode = ELocalTTSErrorCode::None;
	ELocalTTSServiceState ServiceState = ELocalTTSServiceState::Stopped;
	bool bHasLastHealthResponse = false;
	bool bIsServiceReady = false;
	bool bIsTTSRequestInFlight = false;
	static constexpr int32 MaxTTSResponseHistory = 20;
};
