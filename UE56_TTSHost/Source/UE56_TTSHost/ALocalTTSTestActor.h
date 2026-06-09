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

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "LocalTTS Test", meta = (DisplayName = "Health"))
	void CheckLocalTTSHealth();

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "LocalTTS Test", meta = (DisplayName = "Start"))
	void StartLocalTTSService();

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "LocalTTS Test", meta = (DisplayName = "Speak"))
	void SpeakLocalTTSTest();

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "LocalTTS Test", meta = (DisplayName = "Gen"))
	void GenerateLocalTTSTest();

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "LocalTTS Test", meta = (DisplayName = "Stop"))
	void StopLocalTTSSpeaking();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LocalTTS Test")
	FLocalTTSSpeakRequest SpeakRequest;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LocalTTS Test|Auto Run")
	bool bCheckHealthOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LocalTTS Test|Auto Run")
	bool bStartServiceOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LocalTTS Test|Auto Run")
	bool bSpeakOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LocalTTS Test|Auto Run", meta = (ClampMin = "0.1"))
	float AutoSpeakDelaySeconds = 3.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LocalTTS Test|Result")
	FLocalTTSHealthResponse LastHealthResponse;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LocalTTS Test|Result")
	FLocalTTSTTSResponse LastTTSResponse;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LocalTTS Test|Result")
	FLocalTTSSpeechEvent LastSpeechEvent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LocalTTS Test|Result")
	FString LastErrorMessage;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LocalTTS Test|Result")
	FString LastHealthSummary;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LocalTTS Test|Result")
	FString LastTTSSummary;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LocalTTS Test|Result")
	FString LastSpeechEventSummary;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LocalTTS Test|Result")
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
