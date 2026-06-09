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
	bool IsSpeaking() const;
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

	void PlaySpeechAtActor(
		UObject* WorldContextObject,
		const FLocalTTSTTSResponse& TTSResponse,
		AActor* PlaybackActor,
		TFunction<void()>&& OnAudioReady,
		TFunction<void()>&& OnFinished,
		TFunction<void(const FString&)>&& OnFailure);

	UFUNCTION(BlueprintPure, Category = "LocalTTS")
	FString GetServiceBaseUrl() const;

	UFUNCTION(BlueprintPure, Category = "LocalTTS")
	bool IsServiceReady() const;

	UFUNCTION(BlueprintPure, Category = "LocalTTS")
	ELocalTTSServiceState GetServiceState() const;

	UFUNCTION(BlueprintPure, Category = "LocalTTS")
	FString GetServiceStateText() const;

	UFUNCTION(BlueprintPure, Category = "LocalTTS")
	FLocalTTSHealthResponse GetLastHealthResponse() const;

	UFUNCTION(BlueprintPure, Category = "LocalTTS")
	FString GetLastHealthError() const;

	UFUNCTION(BlueprintPure, Category = "LocalTTS")
	ELocalTTSErrorCode GetLastHealthErrorCode() const;

	UFUNCTION(BlueprintPure, Category = "LocalTTS")
	FLocalTTSTTSResponse GetLastTTSResponse() const;

	UFUNCTION(BlueprintPure, Category = "LocalTTS")
	FString GetLastTTSError() const;

	UFUNCTION(BlueprintPure, Category = "LocalTTS")
	ELocalTTSErrorCode GetLastTTSErrorCode() const;

private:
	void UpdateHealthState(const FLocalTTSHealthResponse& Response);
	void UpdateHealthFailure(const FString& ErrorMessage, ELocalTTSErrorCode ErrorCode = ELocalTTSErrorCode::InternalError);
	void UpdateTTSState(const FLocalTTSTTSResponse& Response);
	void UpdateTTSFailure(const FString& ErrorMessage, ELocalTTSErrorCode ErrorCode = ELocalTTSErrorCode::InternalError);

	TSharedPtr<FLocalTTSHttpClient> HttpClient;
	TUniquePtr<FLocalTTSRequestValidator> RequestValidator;
	TUniquePtr<FLocalTTSServiceProcess> ServiceProcess;
	TUniquePtr<FLocalTTSWavLoader> WavLoader;

	UPROPERTY(Transient)
	TObjectPtr<ULocalTTSAudioPlayer> AudioPlayer;

	FLocalTTSHealthResponse LastHealthResponse;
	FLocalTTSTTSResponse LastTTSResponse;
	FString LastHealthError;
	FString LastTTSError;
	ELocalTTSErrorCode LastHealthErrorCode = ELocalTTSErrorCode::None;
	ELocalTTSErrorCode LastTTSErrorCode = ELocalTTSErrorCode::None;
	ELocalTTSServiceState ServiceState = ELocalTTSServiceState::Stopped;
	bool bHasLastHealthResponse = false;
	bool bIsServiceReady = false;
	bool bIsTTSRequestInFlight = false;
};
