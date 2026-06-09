// Copyright Epic Games, Inc. All Rights Reserved.

#include "ULocalTTSSubsystem.h"

#include "FLocalTTSHttpClient.h"
#include "FLocalTTSRequestValidator.h"
#include "FLocalTTSServiceProcess.h"
#include "FLocalTTSWavLoader.h"
#include "Sound/SoundWaveProcedural.h"
#include "ULocalTTSAudioPlayer.h"
#include "ULocalTTSSettings.h"

ULocalTTSSubsystem::ULocalTTSSubsystem() = default;

ULocalTTSSubsystem::~ULocalTTSSubsystem() = default;

void ULocalTTSSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	HttpClient = MakeShared<FLocalTTSHttpClient>();
	RequestValidator = MakeUnique<FLocalTTSRequestValidator>();
	ServiceProcess = MakeUnique<FLocalTTSServiceProcess>();
	WavLoader = MakeUnique<FLocalTTSWavLoader>();
	AudioPlayer = NewObject<ULocalTTSAudioPlayer>(this);
}

void ULocalTTSSubsystem::Deinitialize()
{
	if (AudioPlayer)
	{
		AudioPlayer->Stop();
		AudioPlayer = nullptr;
	}

	WavLoader.Reset();

	if (ServiceProcess.IsValid())
	{
		ServiceProcess->StopService();
		ServiceProcess.Reset();
	}

	RequestValidator.Reset();
	HttpClient.Reset();
	Super::Deinitialize();
}

void ULocalTTSSubsystem::CheckHealth(
	TFunction<void(const FLocalTTSHealthResponse&)>&& OnSuccess,
	TFunction<void(const FString&)>&& OnFailure)
{
	if (!HttpClient.IsValid())
	{
		const FString ErrorMessage = TEXT("LocalTTS HTTP client is not initialized.");
		UpdateHealthFailure(ErrorMessage);
		OnFailure(ErrorMessage);
		return;
	}

	HttpClient->GetHealth(
		GetServiceBaseUrl(),
		[this, OnSuccess = MoveTemp(OnSuccess)](const FLocalTTSHealthResponse& Response) mutable
		{
			UpdateHealthState(Response);
			OnSuccess(Response);
		},
		[this, OnFailure = MoveTemp(OnFailure)](const FString& ErrorMessage) mutable
		{
			UpdateHealthFailure(ErrorMessage);
			OnFailure(ErrorMessage);
		});
}

bool ULocalTTSSubsystem::StartService(FString& OutErrorMessage)
{
	if (!ServiceProcess.IsValid())
	{
		OutErrorMessage = TEXT("LocalTTS service process manager is not initialized.");
		return false;
	}

	return ServiceProcess->StartService(OutErrorMessage);
}

void ULocalTTSSubsystem::StopService()
{
	if (ServiceProcess.IsValid())
	{
		ServiceProcess->StopService();
	}

	StopSpeaking();

	UpdateHealthFailure(TEXT("LocalTTS service stopped."));
	UpdateTTSFailure(TEXT("LocalTTS service stopped."));
}

void ULocalTTSSubsystem::StopSpeaking()
{
	if (AudioPlayer)
	{
		AudioPlayer->Stop();
	}
}

bool ULocalTTSSubsystem::IsSpeaking() const
{
	return AudioPlayer && AudioPlayer->IsPlaying();
}

void ULocalTTSSubsystem::SpeakText(
	const FLocalTTSSpeakRequest& SpeakRequest,
	TFunction<void(const FLocalTTSTTSResponse&)>&& OnSuccess,
	TFunction<void(const FString&)>&& OnFailure)
{
	if (!RequestValidator.IsValid())
	{
		const FString ErrorMessage = TEXT("LocalTTS request validator is not initialized.");
		UpdateTTSFailure(ErrorMessage);
		OnFailure(ErrorMessage);
		return;
	}

	FString ValidationError;
	if (!RequestValidator->Validate(SpeakRequest, ValidationError))
	{
		UpdateTTSFailure(ValidationError);
		OnFailure(ValidationError);
		return;
	}

	if (!HttpClient.IsValid())
	{
		const FString ErrorMessage = TEXT("LocalTTS HTTP client is not initialized.");
		UpdateTTSFailure(ErrorMessage);
		OnFailure(ErrorMessage);
		return;
	}

	HttpClient->PostTTS(
		GetServiceBaseUrl(),
		SpeakRequest,
		[this, OnSuccess = MoveTemp(OnSuccess)](const FLocalTTSTTSResponse& Response) mutable
		{
			UpdateTTSState(Response);
			OnSuccess(Response);
		},
		[this, OnFailure = MoveTemp(OnFailure)](const FString& ErrorMessage) mutable
		{
			UpdateTTSFailure(ErrorMessage);
			OnFailure(ErrorMessage);
		});
}

void ULocalTTSSubsystem::PlaySpeech(
	UObject* WorldContextObject,
	const FLocalTTSTTSResponse& TTSResponse,
	TFunction<void()>&& OnAudioReady,
	TFunction<void()>&& OnFinished,
	TFunction<void(const FString&)>&& OnFailure)
{
	if (!WavLoader.IsValid())
	{
		const FString ErrorMessage = TEXT("LocalTTS WAV loader is not initialized.");
		UpdateTTSFailure(ErrorMessage);
		OnFailure(ErrorMessage);
		return;
	}

	if (!AudioPlayer)
	{
		const FString ErrorMessage = TEXT("LocalTTS audio player is not initialized.");
		UpdateTTSFailure(ErrorMessage);
		OnFailure(ErrorMessage);
		return;
	}

	FString ValidationError;
	if (!WavLoader->ValidateWavFile(TTSResponse.WavPath, ValidationError))
	{
		UpdateTTSFailure(ValidationError);
		OnFailure(ValidationError);
		return;
	}

	FString LoadError;
	USoundWaveProcedural* SoundWave = WavLoader->LoadSoundWave(this, TTSResponse.WavPath, LoadError);
	if (!SoundWave)
	{
		UpdateTTSFailure(LoadError);
		OnFailure(LoadError);
		return;
	}

	FString PlayError;
	if (!AudioPlayer->PlaySound(
		WorldContextObject,
		SoundWave,
		MoveTemp(OnFinished),
		PlayError))
	{
		UpdateTTSFailure(PlayError);
		OnFailure(PlayError);
		return;
	}

	OnAudioReady();
}

FString ULocalTTSSubsystem::GetServiceBaseUrl() const
{
	const ULocalTTSSettings* Settings = GetDefault<ULocalTTSSettings>();
	return Settings ? Settings->ServiceBaseUrl : TEXT("http://127.0.0.1:50021");
}

bool ULocalTTSSubsystem::IsServiceReady() const
{
	return bHasLastHealthResponse && bIsServiceReady;
}

FLocalTTSHealthResponse ULocalTTSSubsystem::GetLastHealthResponse() const
{
	return LastHealthResponse;
}

FString ULocalTTSSubsystem::GetLastHealthError() const
{
	return LastHealthError;
}

FLocalTTSTTSResponse ULocalTTSSubsystem::GetLastTTSResponse() const
{
	return LastTTSResponse;
}

FString ULocalTTSSubsystem::GetLastTTSError() const
{
	return LastTTSError;
}

void ULocalTTSSubsystem::UpdateHealthState(const FLocalTTSHealthResponse& Response)
{
	LastHealthResponse = Response;
	LastHealthError.Reset();
	bHasLastHealthResponse = true;
	bIsServiceReady = Response.bOk && Response.Status.Equals(TEXT("ready"), ESearchCase::IgnoreCase);
}

void ULocalTTSSubsystem::UpdateHealthFailure(const FString& ErrorMessage)
{
	LastHealthResponse = FLocalTTSHealthResponse();
	LastHealthError = ErrorMessage;
	bHasLastHealthResponse = false;
	bIsServiceReady = false;
}

void ULocalTTSSubsystem::UpdateTTSState(const FLocalTTSTTSResponse& Response)
{
	LastTTSResponse = Response;
	LastTTSError.Reset();
}

void ULocalTTSSubsystem::UpdateTTSFailure(const FString& ErrorMessage)
{
	LastTTSResponse = FLocalTTSTTSResponse();
	LastTTSError = ErrorMessage;
}
