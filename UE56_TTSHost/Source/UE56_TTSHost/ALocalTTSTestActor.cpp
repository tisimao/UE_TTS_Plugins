// Copyright Epic Games, Inc. All Rights Reserved.

#include "ALocalTTSTestActor.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "TimerManager.h"

#include "ULocalTTSSettings.h"
#include "ULocalTTSSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogLocalTTSTestActor, Log, All);

ALocalTTSTestActor::ALocalTTSTestActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SpeakRequest.Text = TEXT("Hello, this is a LocalTTS test in UE 5.6.");
	SpeakRequest.Mode = TEXT("auto");
	SpeakRequest.LanguageId = TEXT("zh");
	SpeakRequest.Speed = 1.0f;
}

void ALocalTTSTestActor::BeginPlay()
{
	Super::BeginPlay();

	if (bCheckHealthOnBeginPlay)
	{
		CheckLocalTTSHealth();
	}

	if (bStartServiceOnBeginPlay)
	{
		StartLocalTTSService();
	}

	if (bSpeakOnBeginPlay)
	{
		if (UWorld* World = GetWorld())
		{
			FTimerHandle TimerHandle;
			World->GetTimerManager().SetTimer(
				TimerHandle,
				this,
				&ALocalTTSTestActor::ExecuteDeferredSpeak,
				FMath::Max(0.1f, AutoSpeakDelaySeconds),
				false);
		}
	}
}

void ALocalTTSTestActor::CheckLocalTTSHealth()
{
	ULocalTTSSubsystem* Subsystem = ResolveSubsystem();
	if (!Subsystem)
	{
		StoreError(TEXT("LocalTTS subsystem is unavailable. Run this actor in Play mode so a GameInstance exists."));
		return;
	}

	TWeakObjectPtr<ALocalTTSTestActor> WeakThis(this);
	Subsystem->CheckHealth(
		[WeakThis](const FLocalTTSHealthResponse& Response)
		{
			if (!WeakThis.IsValid())
			{
				return;
			}

			WeakThis->LastHealthResponse = Response;
			WeakThis->LastErrorMessage.Reset();
			WeakThis->LastHealthSummary = FString::Printf(
				TEXT("ok=%s status=%s model=%s"),
				Response.bOk ? TEXT("true") : TEXT("false"),
				*Response.Status,
				*Response.Model);
			UE_LOG(
				LogLocalTTSTestActor,
				Log,
				TEXT("LocalTTS health ok=%s status=%s model=%s"),
				Response.bOk ? TEXT("true") : TEXT("false"),
				*Response.Status,
				*Response.Model);
		},
		[WeakThis](const FString& ErrorMessage)
		{
			if (WeakThis.IsValid())
			{
				WeakThis->StoreError(ErrorMessage);
			}
		});
}

void ALocalTTSTestActor::StartLocalTTSService()
{
	ULocalTTSSubsystem* Subsystem = ResolveSubsystem();
	if (!Subsystem)
	{
		StoreError(TEXT("LocalTTS subsystem is unavailable. Run this actor in Play mode so a GameInstance exists."));
		return;
	}

	FString ErrorMessage;
	if (!Subsystem->StartService(ErrorMessage))
	{
		StoreError(ErrorMessage);
		return;
	}

	LastErrorMessage.Reset();
	UE_LOG(LogLocalTTSTestActor, Log, TEXT("Requested LocalTTS service startup."));
}

void ALocalTTSTestActor::SpeakLocalTTSTest()
{
	ULocalTTSSubsystem* Subsystem = ResolveSubsystem();
	if (!Subsystem)
	{
		StoreError(TEXT("LocalTTS subsystem is unavailable. Run this actor in Play mode so a GameInstance exists."));
		return;
	}

	BeginSpeakFlow();
}

void ALocalTTSTestActor::GenerateLocalTTSTest()
{
	ULocalTTSSubsystem* Subsystem = ResolveSubsystem();
	if (!Subsystem)
	{
		StoreError(TEXT("LocalTTS subsystem is unavailable. Run this actor in Play mode so a GameInstance exists."));
		return;
	}

	BeginGenerateFlow();
}

void ALocalTTSTestActor::StopLocalTTSSpeaking()
{
	ULocalTTSSubsystem* Subsystem = ResolveSubsystem();
	if (!Subsystem)
	{
		StoreError(TEXT("LocalTTS subsystem is unavailable. Run this actor in Play mode so a GameInstance exists."));
		return;
	}

	Subsystem->StopSpeaking();
	LastErrorMessage.Reset();
	UE_LOG(LogLocalTTSTestActor, Log, TEXT("Requested LocalTTS playback stop."));
}

ULocalTTSSubsystem* ALocalTTSTestActor::ResolveSubsystem() const
{
	if (!GEngine)
	{
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return nullptr;
	}

	UGameInstance* GameInstance = World->GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<ULocalTTSSubsystem>() : nullptr;
}

void ALocalTTSTestActor::ExecuteDeferredSpeak()
{
	BeginSpeakFlow();
}

void ALocalTTSTestActor::BeginSpeakFlow()
{
	ULocalTTSSubsystem* Subsystem = ResolveSubsystem();
	if (!Subsystem)
	{
		StoreError(TEXT("LocalTTS subsystem is unavailable. Run this actor in Play mode so a GameInstance exists."));
		return;
	}

	TWeakObjectPtr<ALocalTTSTestActor> WeakThis(this);
	Subsystem->CheckHealth(
		[WeakThis](const FLocalTTSHealthResponse& Response)
		{
			if (!WeakThis.IsValid())
			{
				return;
			}

			WeakThis->LastHealthResponse = Response;
			WeakThis->LastHealthSummary = FString::Printf(
				TEXT("ok=%s status=%s model=%s"),
				Response.bOk ? TEXT("true") : TEXT("false"),
				*Response.Status,
				*Response.Model);
			if (Response.bOk && Response.Status.Equals(TEXT("ready"), ESearchCase::IgnoreCase))
			{
				WeakThis->ExecuteSpeakRequest();
				return;
			}

			FString ErrorMessage;
			ULocalTTSSubsystem* Subsystem = WeakThis->ResolveSubsystem();
			if (!Subsystem)
			{
				WeakThis->StoreError(TEXT("LocalTTS subsystem became unavailable before startup."));
				return;
			}

			if (!Subsystem->StartService(ErrorMessage))
			{
				WeakThis->StoreError(ErrorMessage);
				return;
			}

			WeakThis->BeginHealthPolling();
		},
		[WeakThis](const FString&)
		{
			if (!WeakThis.IsValid())
			{
				return;
			}

			FString ErrorMessage;
			ULocalTTSSubsystem* Subsystem = WeakThis->ResolveSubsystem();
			if (!Subsystem)
			{
				WeakThis->StoreError(TEXT("LocalTTS subsystem became unavailable before startup."));
				return;
			}

			if (!Subsystem->StartService(ErrorMessage))
			{
				WeakThis->StoreError(ErrorMessage);
				return;
			}

			WeakThis->BeginHealthPolling();
		});
}

void ALocalTTSTestActor::BeginGenerateFlow()
{
	ULocalTTSSubsystem* Subsystem = ResolveSubsystem();
	if (!Subsystem)
	{
		StoreError(TEXT("LocalTTS subsystem is unavailable. Run this actor in Play mode so a GameInstance exists."));
		return;
	}

	TWeakObjectPtr<ALocalTTSTestActor> WeakThis(this);
	Subsystem->CheckHealth(
		[WeakThis](const FLocalTTSHealthResponse& Response)
		{
			if (!WeakThis.IsValid())
			{
				return;
			}

			WeakThis->LastHealthResponse = Response;
			WeakThis->LastHealthSummary = FString::Printf(
				TEXT("ok=%s status=%s model=%s"),
				Response.bOk ? TEXT("true") : TEXT("false"),
				*Response.Status,
				*Response.Model);
			if (Response.bOk && Response.Status.Equals(TEXT("ready"), ESearchCase::IgnoreCase))
			{
				WeakThis->ExecuteGenerateRequest();
				return;
			}

			FString ErrorMessage;
			ULocalTTSSubsystem* Subsystem = WeakThis->ResolveSubsystem();
			if (!Subsystem)
			{
				WeakThis->StoreError(TEXT("LocalTTS subsystem became unavailable before startup."));
				return;
			}

			if (!Subsystem->StartService(ErrorMessage))
			{
				WeakThis->StoreError(ErrorMessage);
				return;
			}

			WeakThis->BeginHealthPollingForGenerate();
		},
		[WeakThis](const FString&)
		{
			if (!WeakThis.IsValid())
			{
				return;
			}

			FString ErrorMessage;
			ULocalTTSSubsystem* Subsystem = WeakThis->ResolveSubsystem();
			if (!Subsystem)
			{
				WeakThis->StoreError(TEXT("LocalTTS subsystem became unavailable before startup."));
				return;
			}

			if (!Subsystem->StartService(ErrorMessage))
			{
				WeakThis->StoreError(ErrorMessage);
				return;
			}

			WeakThis->BeginHealthPollingForGenerate();
		});
}

void ALocalTTSTestActor::ExecuteSpeakRequest()
{
	ULocalTTSSubsystem* Subsystem = ResolveSubsystem();
	if (!Subsystem)
	{
		StoreError(TEXT("LocalTTS subsystem became unavailable before speech request."));
		return;
	}

	TWeakObjectPtr<ALocalTTSTestActor> WeakThis(this);
	Subsystem->SpeakText(
		SpeakRequest,
		[WeakThis](const FLocalTTSTTSResponse& Response)
		{
			if (!WeakThis.IsValid())
			{
				return;
			}

			WeakThis->LastTTSResponse = Response;
			WeakThis->LastGeneratedWavPath = Response.WavPath;
			WeakThis->LastTTSSummary = FString::Printf(
				TEXT("ok=%s request_id=%s wav=%s duration_ms=%d"),
				Response.bOk ? TEXT("true") : TEXT("false"),
				*Response.RequestId,
				*Response.WavPath,
				Response.DurationMs);
			WeakThis->LastErrorMessage.Reset();

			ULocalTTSSubsystem* Subsystem = WeakThis->ResolveSubsystem();
			if (!Subsystem)
			{
				WeakThis->StoreError(TEXT("LocalTTS subsystem became unavailable before playback."));
				return;
			}

			Subsystem->PlaySpeech(
				WeakThis.Get(),
				Response,
				[WeakThis]()
				{
					if (WeakThis.IsValid())
					{
						UE_LOG(LogLocalTTSTestActor, Log, TEXT("LocalTTS audio playback started."));
					}
				},
				[WeakThis]()
				{
					if (WeakThis.IsValid())
					{
						UE_LOG(LogLocalTTSTestActor, Log, TEXT("LocalTTS audio playback finished."));
					}
				},
				[WeakThis](const FString& ErrorMessage)
				{
					if (WeakThis.IsValid())
					{
						WeakThis->StoreError(ErrorMessage);
					}
				});
		},
		[WeakThis](const FString& ErrorMessage)
		{
			if (WeakThis.IsValid())
			{
				WeakThis->StoreError(ErrorMessage);
			}
		});
}

void ALocalTTSTestActor::ExecuteGenerateRequest()
{
	ULocalTTSSubsystem* Subsystem = ResolveSubsystem();
	if (!Subsystem)
	{
		StoreError(TEXT("LocalTTS subsystem became unavailable before speech generation."));
		return;
	}

	TWeakObjectPtr<ALocalTTSTestActor> WeakThis(this);
	Subsystem->SpeakText(
		SpeakRequest,
		[WeakThis](const FLocalTTSTTSResponse& Response)
		{
			if (!WeakThis.IsValid())
			{
				return;
			}

			WeakThis->LastTTSResponse = Response;
			WeakThis->LastSpeechEvent = FLocalTTSSpeechEvent::FromRequestAndResponse(WeakThis->SpeakRequest, Response);
			WeakThis->LastGeneratedWavPath = Response.WavPath;
			WeakThis->LastTTSSummary = FString::Printf(
				TEXT("ok=%s request_id=%s wav=%s duration_ms=%d"),
				Response.bOk ? TEXT("true") : TEXT("false"),
				*Response.RequestId,
				*Response.WavPath,
				Response.DurationMs);
			WeakThis->LastSpeechEventSummary = FString::Printf(
				TEXT("request_id=%s text=%s wav=%s duration_sec=%.2f visemes=%d emotions=%d"),
				*WeakThis->LastSpeechEvent.RequestId,
				*WeakThis->LastSpeechEvent.Text,
				*WeakThis->LastSpeechEvent.WavPath,
				WeakThis->LastSpeechEvent.DurationSeconds,
				WeakThis->LastSpeechEvent.VisemeFrames.Num(),
				WeakThis->LastSpeechEvent.EmotionFrames.Num());
			WeakThis->LastErrorMessage.Reset();
			UE_LOG(
				LogLocalTTSTestActor,
				Log,
				TEXT("LocalTTS generated speech event request_id=%s wav=%s"),
				*WeakThis->LastSpeechEvent.RequestId,
				*WeakThis->LastSpeechEvent.WavPath);
		},
		[WeakThis](const FString& ErrorMessage)
		{
			if (WeakThis.IsValid())
			{
				WeakThis->StoreError(ErrorMessage);
			}
		});
}

void ALocalTTSTestActor::BeginHealthPolling()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		StoreError(TEXT("Failed to resolve world while waiting for LocalTTS service."));
		return;
	}

	const ULocalTTSSettings* Settings = GetDefault<ULocalTTSSettings>();
	const int32 MaxHealthPollCount = Settings ? Settings->MaxHealthPollCount : 30;
	const float HealthPollIntervalSeconds = Settings ? Settings->HealthPollIntervalSeconds : 1.0f;

	RemainingHealthPollCount = FMath::Max(1, MaxHealthPollCount);
	World->GetTimerManager().SetTimer(
		HealthPollTimerHandle,
		this,
		&ALocalTTSTestActor::PollServiceHealth,
		FMath::Max(0.1f, HealthPollIntervalSeconds),
		true);
}

void ALocalTTSTestActor::PollServiceHealth()
{
	ULocalTTSSubsystem* Subsystem = ResolveSubsystem();
	if (!Subsystem)
	{
		StoreError(TEXT("LocalTTS subsystem is unavailable while waiting for service readiness."));
		return;
	}

	if (RemainingHealthPollCount <= 0)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(HealthPollTimerHandle);
		}
		StoreError(TEXT("Timed out while waiting for LocalTTS service to become ready."));
		return;
	}

	--RemainingHealthPollCount;

	TWeakObjectPtr<ALocalTTSTestActor> WeakThis(this);
	Subsystem->CheckHealth(
		[WeakThis](const FLocalTTSHealthResponse& Response)
		{
			if (!WeakThis.IsValid())
			{
				return;
			}

			WeakThis->LastHealthResponse = Response;
			WeakThis->LastHealthSummary = FString::Printf(
				TEXT("ok=%s status=%s model=%s"),
				Response.bOk ? TEXT("true") : TEXT("false"),
				*Response.Status,
				*Response.Model);
			if (!Response.bOk || !Response.Status.Equals(TEXT("ready"), ESearchCase::IgnoreCase))
			{
				return;
			}

			if (UWorld* World = WeakThis->GetWorld())
			{
				World->GetTimerManager().ClearTimer(WeakThis->HealthPollTimerHandle);
			}

			WeakThis->ExecuteSpeakRequest();
		},
		[](const FString&)
		{
		});
}

void ALocalTTSTestActor::BeginHealthPollingForGenerate()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		StoreError(TEXT("Failed to resolve world while waiting for LocalTTS service."));
		return;
	}

	const ULocalTTSSettings* Settings = GetDefault<ULocalTTSSettings>();
	const int32 MaxHealthPollCount = Settings ? Settings->MaxHealthPollCount : 30;
	const float HealthPollIntervalSeconds = Settings ? Settings->HealthPollIntervalSeconds : 1.0f;

	RemainingHealthPollCount = FMath::Max(1, MaxHealthPollCount);
	World->GetTimerManager().SetTimer(
		HealthPollTimerHandle,
		this,
		&ALocalTTSTestActor::PollServiceHealthForGenerate,
		FMath::Max(0.1f, HealthPollIntervalSeconds),
		true);
}

void ALocalTTSTestActor::PollServiceHealthForGenerate()
{
	ULocalTTSSubsystem* Subsystem = ResolveSubsystem();
	if (!Subsystem)
	{
		StoreError(TEXT("LocalTTS subsystem is unavailable while waiting for service readiness."));
		return;
	}

	if (RemainingHealthPollCount <= 0)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(HealthPollTimerHandle);
		}
		StoreError(TEXT("Timed out while waiting for LocalTTS service to become ready."));
		return;
	}

	--RemainingHealthPollCount;

	TWeakObjectPtr<ALocalTTSTestActor> WeakThis(this);
	Subsystem->CheckHealth(
		[WeakThis](const FLocalTTSHealthResponse& Response)
		{
			if (!WeakThis.IsValid())
			{
				return;
			}

			WeakThis->LastHealthResponse = Response;
			WeakThis->LastHealthSummary = FString::Printf(
				TEXT("ok=%s status=%s model=%s"),
				Response.bOk ? TEXT("true") : TEXT("false"),
				*Response.Status,
				*Response.Model);
			if (!Response.bOk || !Response.Status.Equals(TEXT("ready"), ESearchCase::IgnoreCase))
			{
				return;
			}

			if (UWorld* World = WeakThis->GetWorld())
			{
				World->GetTimerManager().ClearTimer(WeakThis->HealthPollTimerHandle);
			}

			WeakThis->ExecuteGenerateRequest();
		},
		[](const FString&)
		{
		});
}

void ALocalTTSTestActor::StoreError(const FString& ErrorMessage)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HealthPollTimerHandle);
	}

	LastErrorMessage = ErrorMessage;
	LastTTSSummary.Reset();
	UE_LOG(LogLocalTTSTestActor, Error, TEXT("%s"), *ErrorMessage);
}
