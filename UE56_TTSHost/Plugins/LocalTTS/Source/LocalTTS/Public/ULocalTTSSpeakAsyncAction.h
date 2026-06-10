// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"

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

UCLASS()
class LOCALTTS_API ULocalTTSSpeakAsyncAction : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "LocalTTS|Speech")
	FLocalTTSSpeakStartedDelegate OnStarted;

	UPROPERTY(BlueprintAssignable, Category = "LocalTTS|Speech")
	FLocalTTSSpeakSuccessDelegate OnSucceeded;

	UPROPERTY(BlueprintAssignable, Category = "LocalTTS|Speech")
	FLocalTTSAudioReadyDelegate OnAudioReady;

	UPROPERTY(BlueprintAssignable, Category = "LocalTTS|Speech")
	FLocalTTSSpeechEventReadyDelegate OnSpeechEventReady;

	UPROPERTY(BlueprintAssignable, Category = "LocalTTS|Speech")
	FLocalTTSSpeakFinishedDelegate OnFinished;

	UPROPERTY(BlueprintAssignable, Category = "LocalTTS|Speech")
	FLocalTTSSpeakFailureDelegate OnError;

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "生成并播放 Local TTS", ToolTip = "调用本地 OmniVoice 服务生成 wav，并在 UE 中以 2D 声音播放。适合普通旁白、UI 语音和快速测试。"), Category = "LocalTTS|Speech")
	static ULocalTTSSpeakAsyncAction* SpeakTextAsync(UObject* WorldContextObject, const FLocalTTSSpeakRequest& SpeakRequest);

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "在 Actor 位置播放 Local TTS", ToolTip = "调用本地 OmniVoice 服务生成 wav，并绑定到指定 Actor 位置进行 3D 播放。适合场景角色、数字人或空间音频测试。"), Category = "LocalTTS|Speech")
	static ULocalTTSSpeakAsyncAction* SpeakTextAtActorAsync(UObject* WorldContextObject, const FLocalTTSSpeakRequest& SpeakRequest, AActor* PlaybackActor);

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "只生成 Local TTS", ToolTip = "只调用本地 OmniVoice 服务生成 wav，不自动播放。适合把 wav 路径交给自定义播放、口型分析或数字人表情驱动系统。"), Category = "LocalTTS|Speech")
	static ULocalTTSSpeakAsyncAction* GenerateSpeechAsync(UObject* WorldContextObject, const FLocalTTSSpeakRequest& SpeakRequest);

	virtual void Activate() override;

private:
	void TrySendRequest();
	void BeginHealthPolling();
	void PollServiceHealth();
	void HandleSpeechResponse(const FLocalTTSTTSResponse& Response);
	void HandlePlaybackFinished();
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
