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
		const FString ErrorMessage = TEXT("LocalTTS HTTP 客户端尚未初始化。");
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
		OutErrorMessage = TEXT("LocalTTS 服务进程管理器尚未初始化。");
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

	UpdateHealthFailure(TEXT("LocalTTS 服务已停止。"), ELocalTTSErrorCode::None);
	UpdateTTSFailure(TEXT("LocalTTS 服务已停止。"), ELocalTTSErrorCode::None);
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

bool ULocalTTSSubsystem::PauseSpeaking(FString& OutErrorMessage)
{
	if (!AudioPlayer)
	{
		OutErrorMessage = TEXT("LocalTTS 音频播放器尚未初始化。");
		UpdateTTSFailure(OutErrorMessage, ELocalTTSErrorCode::InternalError);
		return false;
	}

	return AudioPlayer->Pause(OutErrorMessage);
}

bool ULocalTTSSubsystem::ResumeSpeaking(FString& OutErrorMessage)
{
	if (!AudioPlayer)
	{
		OutErrorMessage = TEXT("LocalTTS 音频播放器尚未初始化。");
		UpdateTTSFailure(OutErrorMessage, ELocalTTSErrorCode::InternalError);
		return false;
	}

	return AudioPlayer->Resume(OutErrorMessage);
}

bool ULocalTTSSubsystem::IsSpeaking() const
{
	return AudioPlayer && AudioPlayer->IsPlaying();
}

bool ULocalTTSSubsystem::IsSpeakingPaused() const
{
	return AudioPlayer && AudioPlayer->IsPaused();
}

bool ULocalTTSSubsystem::IsTTSRequestInFlight() const
{
	return bIsTTSRequestInFlight;
}

bool ULocalTTSSubsystem::IsBusy() const
{
	return IsTTSRequestInFlight() || IsSpeaking() || IsSpeakingPaused();
}

void ULocalTTSSubsystem::SpeakText(
	const FLocalTTSSpeakRequest& SpeakRequest,
	TFunction<void(const FLocalTTSTTSResponse&)>&& OnSuccess,
	TFunction<void(const FString&)>&& OnFailure)
{
	if (bIsTTSRequestInFlight)
	{
		const FString ErrorMessage = TEXT("LocalTTS 正在处理另一条语音请求。请等待 OnFinished，或先检查 Is Local TTS Busy 再发送新请求。");
		LastTTSError = ErrorMessage;
		LastTTSErrorCode = ELocalTTSErrorCode::AlreadyBusy;
		ServiceState = ELocalTTSServiceState::Busy;
		OnFailure(ErrorMessage);
		return;
	}

	if (!RequestValidator.IsValid())
	{
		const FString ErrorMessage = TEXT("LocalTTS 请求校验器尚未初始化。");
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
		const FString ErrorMessage = TEXT("LocalTTS HTTP 客户端尚未初始化。");
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
		const FString ErrorMessage = TEXT("LocalTTS WAV 加载器尚未初始化。");
		UpdateTTSFailure(ErrorMessage, ELocalTTSErrorCode::InternalError);
		OnFailure(ErrorMessage);
		return;
	}

	if (!AudioPlayer)
	{
		const FString ErrorMessage = TEXT("LocalTTS 音频播放器尚未初始化。");
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
			HandlePlaybackFinished();
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
		const FString ErrorMessage = TEXT("LocalTTS WAV 加载器尚未初始化。");
		UpdateTTSFailure(ErrorMessage, ELocalTTSErrorCode::InternalError);
		OnFailure(ErrorMessage);
		return;
	}

	if (!AudioPlayer)
	{
		const FString ErrorMessage = TEXT("LocalTTS 音频播放器尚未初始化。");
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
			HandlePlaybackFinished();
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
		return TEXT("已停止");
	case ELocalTTSServiceState::Starting:
		return TEXT("启动中");
	case ELocalTTSServiceState::Ready:
		return TEXT("就绪");
	case ELocalTTSServiceState::Busy:
		return TEXT("忙碌");
	case ELocalTTSServiceState::Error:
		return TEXT("错误");
	default:
		return TEXT("未知");
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

void ULocalTTSSubsystem::HandlePlaybackFinished()
{
	bIsTTSRequestInFlight = false;
	ServiceState = IsServiceReady() ? ELocalTTSServiceState::Ready : ELocalTTSServiceState::Starting;
}
