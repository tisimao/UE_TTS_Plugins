// Copyright Epic Games, Inc. All Rights Reserved.

#include "ULocalTTSSubsystem.h"

#include "FLocalTTSHttpClient.h"
#include "FLocalTTSRequestValidator.h"
#include "FLocalTTSServiceProcess.h"
#include "FLocalTTSWavLoader.h"
#include "Sound/SoundWaveProcedural.h"
#include "ULocalTTSAudioPlayer.h"
#include "ULocalTTSSettings.h"

DEFINE_LOG_CATEGORY_STATIC(LogLocalTTSSubsystem, Log, All);

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
		UpdateHealthFailure(ErrorMessage, ELocalTTSErrorCode::InternalError);
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
		[this, OnFailure = MoveTemp(OnFailure)](ELocalTTSErrorCode ErrorCode, const FString& ErrorMessage) mutable
		{
			UpdateHealthFailure(ErrorMessage, ErrorCode);
			OnFailure(ErrorMessage);
		});
}

bool ULocalTTSSubsystem::StartService(FString& OutErrorMessage)
{
	if (!ServiceProcess.IsValid())
	{
		OutErrorMessage = TEXT("LocalTTS service process manager is not initialized.");
		ServiceState = ELocalTTSServiceState::Error;
		LastHealthError = OutErrorMessage;
		LastHealthErrorCode = ELocalTTSErrorCode::InternalError;
		return false;
	}

	if (!ServiceProcess->StartService(OutErrorMessage))
	{
		ServiceState = ELocalTTSServiceState::Error;
		LastHealthError = OutErrorMessage;
		LastHealthErrorCode = ELocalTTSErrorCode::ServiceProcessError;
		return false;
	}

	if (!IsServiceReady())
	{
		ServiceState = ELocalTTSServiceState::Starting;
	}

	return true;
}

void ULocalTTSSubsystem::StopService()
{
	if (ServiceProcess.IsValid())
	{
		ServiceProcess->StopService();
	}

	StopSpeaking();

	UpdateHealthFailure(TEXT("LocalTTS service stopped."), ELocalTTSErrorCode::None);
	UpdateTTSFailure(TEXT("LocalTTS service stopped."), ELocalTTSErrorCode::None);
	ServiceState = ELocalTTSServiceState::Stopped;
}

void ULocalTTSSubsystem::StopSpeaking()
{
	if (AudioPlayer)
	{
		AudioPlayer->Stop();
	}

	if (!bIsTTSRequestInFlight)
	{
		ServiceState = IsServiceReady() ? ELocalTTSServiceState::Ready : ELocalTTSServiceState::Starting;
	}
}

bool ULocalTTSSubsystem::IsSpeaking() const
{
	return AudioPlayer && AudioPlayer->IsPlaying();
}

bool ULocalTTSSubsystem::IsTTSRequestInFlight() const
{
	return bIsTTSRequestInFlight;
}

bool ULocalTTSSubsystem::IsBusy() const
{
	return IsTTSRequestInFlight() || IsSpeaking();
}

void ULocalTTSSubsystem::SpeakText(
	const FLocalTTSSpeakRequest& SpeakRequest,
	TFunction<void(const FLocalTTSTTSResponse&)>&& OnSuccess,
	TFunction<void(const FString&)>&& OnFailure)
{
	if (bIsTTSRequestInFlight)
	{
		const FString ErrorMessage = TEXT("LocalTTS is already processing a speech request.");
		LastTTSError = ErrorMessage;
		LastTTSErrorCode = ELocalTTSErrorCode::AlreadyBusy;
		ServiceState = ELocalTTSServiceState::Busy;
		OnFailure(ErrorMessage);
		return;
	}

	if (!RequestValidator.IsValid())
	{
		const FString ErrorMessage = TEXT("LocalTTS request validator is not initialized.");
		UpdateTTSFailure(ErrorMessage, ELocalTTSErrorCode::InternalError);
		OnFailure(ErrorMessage);
		return;
	}

	FString ValidationError;
	if (!RequestValidator->Validate(SpeakRequest, ValidationError))
	{
		UpdateTTSFailure(ValidationError, ELocalTTSErrorCode::RequestValidationFailed);
		OnFailure(ValidationError);
		return;
	}

	if (!HttpClient.IsValid())
	{
		const FString ErrorMessage = TEXT("LocalTTS HTTP client is not initialized.");
		UpdateTTSFailure(ErrorMessage, ELocalTTSErrorCode::InternalError);
		OnFailure(ErrorMessage);
		return;
	}

	bIsTTSRequestInFlight = true;
	ServiceState = ELocalTTSServiceState::Busy;

	HttpClient->PostTTS(
		GetServiceBaseUrl(),
		SpeakRequest,
		[this, OnSuccess = MoveTemp(OnSuccess)](const FLocalTTSTTSResponse& Response) mutable
		{
			bIsTTSRequestInFlight = false;
			UpdateTTSState(Response);
			UE_LOG(
				LogLocalTTSSubsystem,
				Log,
				TEXT("LocalTTS speech request finished ok=%s request_id=%s wav=%s duration_ms=%d"),
				Response.bOk ? TEXT("true") : TEXT("false"),
				*Response.RequestId,
				*Response.WavPath,
				Response.DurationMs);
			OnSuccess(Response);
		},
		[this, OnFailure = MoveTemp(OnFailure)](ELocalTTSErrorCode ErrorCode, const FString& ErrorMessage) mutable
		{
			bIsTTSRequestInFlight = false;
			UpdateTTSFailure(ErrorMessage, ErrorCode);
			UE_LOG(
				LogLocalTTSSubsystem,
				Error,
				TEXT("LocalTTS speech request failed error_code=%d message=%s"),
				static_cast<int32>(ErrorCode),
				*ErrorMessage);
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
		UpdateTTSFailure(ErrorMessage, ELocalTTSErrorCode::InternalError);
		OnFailure(ErrorMessage);
		return;
	}

	if (!AudioPlayer)
	{
		const FString ErrorMessage = TEXT("LocalTTS audio player is not initialized.");
		UpdateTTSFailure(ErrorMessage, ELocalTTSErrorCode::InternalError);
		OnFailure(ErrorMessage);
		return;
	}

	FString ValidationError;
	if (!WavLoader->ValidateWavFile(TTSResponse.WavPath, ValidationError))
	{
		UpdateTTSFailure(ValidationError, ELocalTTSErrorCode::WavFileInvalid);
		OnFailure(ValidationError);
		return;
	}

	FString LoadError;
	USoundWaveProcedural* SoundWave = WavLoader->LoadSoundWave(this, TTSResponse.WavPath, LoadError);
	if (!SoundWave)
	{
		UpdateTTSFailure(LoadError, ELocalTTSErrorCode::WavLoadFailed);
		OnFailure(LoadError);
		return;
	}

	FString PlayError;
	if (!AudioPlayer->PlaySound(
		WorldContextObject,
		SoundWave,
		[this, OnFinished = MoveTemp(OnFinished)]() mutable
		{
			ServiceState = IsServiceReady() ? ELocalTTSServiceState::Ready : ELocalTTSServiceState::Starting;
			if (OnFinished)
			{
				OnFinished();
			}
		},
		PlayError))
	{
		UpdateTTSFailure(PlayError, ELocalTTSErrorCode::PlaybackFailed);
		OnFailure(PlayError);
		return;
	}

	ServiceState = ELocalTTSServiceState::Busy;
	UE_LOG(
		LogLocalTTSSubsystem,
		Log,
		TEXT("LocalTTS playback started mode=2d request_id=%s wav=%s"),
		*TTSResponse.RequestId,
		*TTSResponse.WavPath);
	OnAudioReady();
}

void ULocalTTSSubsystem::PlaySpeechAtActor(
	UObject* WorldContextObject,
	const FLocalTTSTTSResponse& TTSResponse,
	AActor* PlaybackActor,
	TFunction<void()>&& OnAudioReady,
	TFunction<void()>&& OnFinished,
	TFunction<void(const FString&)>&& OnFailure)
{
	if (!WavLoader.IsValid())
	{
		const FString ErrorMessage = TEXT("LocalTTS WAV loader is not initialized.");
		UpdateTTSFailure(ErrorMessage, ELocalTTSErrorCode::InternalError);
		OnFailure(ErrorMessage);
		return;
	}

	if (!AudioPlayer)
	{
		const FString ErrorMessage = TEXT("LocalTTS audio player is not initialized.");
		UpdateTTSFailure(ErrorMessage, ELocalTTSErrorCode::InternalError);
		OnFailure(ErrorMessage);
		return;
	}

	FString ValidationError;
	if (!WavLoader->ValidateWavFile(TTSResponse.WavPath, ValidationError))
	{
		UpdateTTSFailure(ValidationError, ELocalTTSErrorCode::WavFileInvalid);
		OnFailure(ValidationError);
		return;
	}

	FString LoadError;
	USoundWaveProcedural* SoundWave = WavLoader->LoadSoundWave(this, TTSResponse.WavPath, LoadError);
	if (!SoundWave)
	{
		UpdateTTSFailure(LoadError, ELocalTTSErrorCode::WavLoadFailed);
		OnFailure(LoadError);
		return;
	}

	FString PlayError;
	if (!AudioPlayer->PlaySoundAtActor(
		WorldContextObject,
		SoundWave,
		PlaybackActor,
		[this, OnFinished = MoveTemp(OnFinished)]() mutable
		{
			ServiceState = IsServiceReady() ? ELocalTTSServiceState::Ready : ELocalTTSServiceState::Starting;
			if (OnFinished)
			{
				OnFinished();
			}
		},
		PlayError))
	{
		UpdateTTSFailure(PlayError, ELocalTTSErrorCode::PlaybackFailed);
		OnFailure(PlayError);
		return;
	}

	ServiceState = ELocalTTSServiceState::Busy;
	UE_LOG(
		LogLocalTTSSubsystem,
		Log,
		TEXT("LocalTTS playback started mode=actor request_id=%s wav=%s actor=%s"),
		*TTSResponse.RequestId,
		*TTSResponse.WavPath,
		PlaybackActor ? *PlaybackActor->GetName() : TEXT("None"));
	OnAudioReady();
}

FString ULocalTTSSubsystem::GetServiceBaseUrl() const
{
	const ULocalTTSSettings* Settings = GetDefault<ULocalTTSSettings>();
	FString BaseUrl = Settings ? Settings->ServiceBaseUrl.TrimStartAndEnd() : TEXT("");
	if (BaseUrl.IsEmpty() || !BaseUrl.Contains(TEXT("://")))
	{
		return TEXT("http://127.0.0.1:50021");
	}

	BaseUrl.RemoveFromEnd(TEXT("/"));
	return BaseUrl;
}

bool ULocalTTSSubsystem::IsServiceReady() const
{
	return bHasLastHealthResponse && bIsServiceReady;
}

ELocalTTSServiceState ULocalTTSSubsystem::GetServiceState() const
{
	return ServiceState;
}

FString ULocalTTSSubsystem::GetServiceStateText() const
{
	switch (ServiceState)
	{
	case ELocalTTSServiceState::Stopped:
		return TEXT("Stopped");
	case ELocalTTSServiceState::Starting:
		return TEXT("Starting");
	case ELocalTTSServiceState::Ready:
		return TEXT("Ready");
	case ELocalTTSServiceState::Busy:
		return TEXT("Busy");
	case ELocalTTSServiceState::Error:
		return TEXT("Error");
	default:
		return TEXT("Unknown");
	}
}

FLocalTTSHealthResponse ULocalTTSSubsystem::GetLastHealthResponse() const
{
	return LastHealthResponse;
}

FString ULocalTTSSubsystem::GetLastHealthError() const
{
	return LastHealthError;
}

ELocalTTSErrorCode ULocalTTSSubsystem::GetLastHealthErrorCode() const
{
	return LastHealthErrorCode;
}

FLocalTTSTTSResponse ULocalTTSSubsystem::GetLastTTSResponse() const
{
	return LastTTSResponse;
}

FString ULocalTTSSubsystem::GetLastTTSError() const
{
	return LastTTSError;
}

ELocalTTSErrorCode ULocalTTSSubsystem::GetLastTTSErrorCode() const
{
	return LastTTSErrorCode;
}

void ULocalTTSSubsystem::UpdateHealthState(const FLocalTTSHealthResponse& Response)
{
	LastHealthResponse = Response;
	LastHealthError.Reset();
	LastHealthErrorCode = ELocalTTSErrorCode::None;
	bHasLastHealthResponse = true;
	bIsServiceReady = Response.bOk && Response.Status.Equals(TEXT("ready"), ESearchCase::IgnoreCase);
	ServiceState = bIsServiceReady ? ELocalTTSServiceState::Ready : ELocalTTSServiceState::Starting;
}

void ULocalTTSSubsystem::UpdateHealthFailure(const FString& ErrorMessage, ELocalTTSErrorCode ErrorCode)
{
	LastHealthResponse = FLocalTTSHealthResponse();
	LastHealthError = ErrorMessage;
	LastHealthErrorCode = ErrorCode;
	bHasLastHealthResponse = false;
	bIsServiceReady = false;
	ServiceState = ErrorCode == ELocalTTSErrorCode::None ? ELocalTTSServiceState::Stopped : ELocalTTSServiceState::Error;
}

void ULocalTTSSubsystem::UpdateTTSState(const FLocalTTSTTSResponse& Response)
{
	LastTTSResponse = Response;
	LastTTSError.Reset();
	LastTTSErrorCode = Response.bOk ? ELocalTTSErrorCode::None : ELocalTTSErrorCode::ServiceReturnedError;
	ServiceState = Response.bOk
		? (IsServiceReady() ? ELocalTTSServiceState::Ready : ELocalTTSServiceState::Starting)
		: ELocalTTSServiceState::Error;
}

void ULocalTTSSubsystem::UpdateTTSFailure(const FString& ErrorMessage, ELocalTTSErrorCode ErrorCode)
{
	bIsTTSRequestInFlight = false;
	LastTTSResponse = FLocalTTSTTSResponse();
	LastTTSError = ErrorMessage;
	LastTTSErrorCode = ErrorCode;
	ServiceState = ErrorCode == ELocalTTSErrorCode::None ? ELocalTTSServiceState::Stopped : ELocalTTSServiceState::Error;
}
