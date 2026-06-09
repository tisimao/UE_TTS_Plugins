// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "FLocalTTSHealthResponse.h"
#include "FLocalTTSRequestValidator.h"
#include "FLocalTTSServiceProcess.h"
#include "FLocalTTSSpeakRequest.h"
#include "FLocalTTSTTSResponse.h"
#include "FLocalTTSWavLoader.h"
#include "ULocalTTSSubsystem.generated.h"

class FLocalTTSHttpClient;
class ULocalTTSAudioPlayer;

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

	UFUNCTION(BlueprintPure, Category = "LocalTTS")
	FString GetServiceBaseUrl() const;

	UFUNCTION(BlueprintPure, Category = "LocalTTS")
	bool IsServiceReady() const;

	UFUNCTION(BlueprintPure, Category = "LocalTTS")
	FLocalTTSHealthResponse GetLastHealthResponse() const;

	UFUNCTION(BlueprintPure, Category = "LocalTTS")
	FString GetLastHealthError() const;

	UFUNCTION(BlueprintPure, Category = "LocalTTS")
	FLocalTTSTTSResponse GetLastTTSResponse() const;

	UFUNCTION(BlueprintPure, Category = "LocalTTS")
	FString GetLastTTSError() const;

private:
	void UpdateHealthState(const FLocalTTSHealthResponse& Response);
	void UpdateHealthFailure(const FString& ErrorMessage);
	void UpdateTTSState(const FLocalTTSTTSResponse& Response);
	void UpdateTTSFailure(const FString& ErrorMessage);

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
	bool bHasLastHealthResponse = false;
	bool bIsServiceReady = false;
};
