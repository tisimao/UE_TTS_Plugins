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

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Speak Local TTS Async", ToolTip = "Generate speech through the local OmniVoice service and play the returned wav in UE."), Category = "LocalTTS|Speech")
	static ULocalTTSSpeakAsyncAction* SpeakTextAsync(UObject* WorldContextObject, const FLocalTTSSpeakRequest& SpeakRequest);

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Speak Local TTS At Actor Async", ToolTip = "Generate speech through the local OmniVoice service and play the returned wav at an actor location."), Category = "LocalTTS|Speech")
	static ULocalTTSSpeakAsyncAction* SpeakTextAtActorAsync(UObject* WorldContextObject, const FLocalTTSSpeakRequest& SpeakRequest, AActor* PlaybackActor);

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Generate Local TTS Async", ToolTip = "Generate speech through the local OmniVoice service without playing it. Use the returned wav path for custom playback, lip sync, or digital human driving."), Category = "LocalTTS|Speech")
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
