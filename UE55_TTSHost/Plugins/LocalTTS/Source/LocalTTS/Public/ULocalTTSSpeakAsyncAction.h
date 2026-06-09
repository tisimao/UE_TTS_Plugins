// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"

#include "FLocalTTSSpeakRequest.h"
#include "FLocalTTSTTSResponse.h"
#include "ULocalTTSSpeakAsyncAction.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLocalTTSSpeakStartedDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLocalTTSSpeakSuccessDelegate, const FLocalTTSTTSResponse&, Response);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLocalTTSAudioReadyDelegate, const FLocalTTSTTSResponse&, Response);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLocalTTSSpeakFinishedDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLocalTTSSpeakFailureDelegate, const FString&, ErrorMessage);

UCLASS()
class LOCALTTS_API ULocalTTSSpeakAsyncAction : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FLocalTTSSpeakStartedDelegate OnStarted;

	UPROPERTY(BlueprintAssignable)
	FLocalTTSSpeakSuccessDelegate OnSucceeded;

	UPROPERTY(BlueprintAssignable)
	FLocalTTSAudioReadyDelegate OnAudioReady;

	UPROPERTY(BlueprintAssignable)
	FLocalTTSSpeakFinishedDelegate OnFinished;

	UPROPERTY(BlueprintAssignable)
	FLocalTTSSpeakFailureDelegate OnError;

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"), Category = "LocalTTS")
	static ULocalTTSSpeakAsyncAction* SpeakTextAsync(UObject* WorldContextObject, const FLocalTTSSpeakRequest& SpeakRequest);

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
	FLocalTTSSpeakRequest SpeakRequest;
	TWeakObjectPtr<class ULocalTTSSubsystem> Subsystem;
	FTimerHandle HealthPollTimerHandle;
	int32 RemainingHealthPollCount = 0;
};
