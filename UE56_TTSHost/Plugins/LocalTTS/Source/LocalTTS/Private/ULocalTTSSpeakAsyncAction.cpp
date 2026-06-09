// Copyright Epic Games, Inc. All Rights Reserved.

#include "ULocalTTSSpeakAsyncAction.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"

#include "ULocalTTSSettings.h"
#include "ULocalTTSSubsystem.h"

ULocalTTSSpeakAsyncAction* ULocalTTSSpeakAsyncAction::SpeakTextAsync(
	UObject* WorldContextObject,
	const FLocalTTSSpeakRequest& InSpeakRequest)
{
	ULocalTTSSpeakAsyncAction* Action = NewObject<ULocalTTSSpeakAsyncAction>();
	Action->WorldContextObject = WorldContextObject;
	Action->SpeakRequest = InSpeakRequest;
	return Action;
}

ULocalTTSSpeakAsyncAction* ULocalTTSSpeakAsyncAction::SpeakTextAtActorAsync(
	UObject* WorldContextObject,
	const FLocalTTSSpeakRequest& InSpeakRequest,
	AActor* InPlaybackActor)
{
	ULocalTTSSpeakAsyncAction* Action = NewObject<ULocalTTSSpeakAsyncAction>();
	Action->WorldContextObject = WorldContextObject;
	Action->SpeakRequest = InSpeakRequest;
	Action->PlaybackActor = InPlaybackActor;
	Action->bUseActorPlayback = true;
	return Action;
}

ULocalTTSSpeakAsyncAction* ULocalTTSSpeakAsyncAction::GenerateSpeechAsync(
	UObject* WorldContextObject,
	const FLocalTTSSpeakRequest& InSpeakRequest)
{
	ULocalTTSSpeakAsyncAction* Action = NewObject<ULocalTTSSpeakAsyncAction>();
	Action->WorldContextObject = WorldContextObject;
	Action->SpeakRequest = InSpeakRequest;
	Action->bAutoPlay = false;
	return Action;
}

void ULocalTTSSpeakAsyncAction::Activate()
{
	UObject* ContextObject = WorldContextObject.Get();
	if (!ContextObject)
	{
		FinishWithFailure(TEXT("World context is invalid."));
		return;
	}

	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(ContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World)
	{
		FinishWithFailure(TEXT("Failed to resolve world for LocalTTS speak request."));
		return;
	}

	UGameInstance* GameInstance = World->GetGameInstance();
	if (!GameInstance)
	{
		FinishWithFailure(TEXT("Game instance is unavailable."));
		return;
	}

	RegisterWithGameInstance(GameInstance);

	ULocalTTSSubsystem* LocalSubsystem = GameInstance->GetSubsystem<ULocalTTSSubsystem>();
	if (!LocalSubsystem)
	{
		FinishWithFailure(TEXT("LocalTTS subsystem is unavailable."));
		return;
	}

	Subsystem = LocalSubsystem;
	OnStarted.Broadcast();

	TrySendRequest();
}

void ULocalTTSSpeakAsyncAction::TrySendRequest()
{
	if (!Subsystem.IsValid())
	{
		FinishWithFailure(TEXT("LocalTTS subsystem is unavailable."));
		return;
	}

	TWeakObjectPtr<ULocalTTSSpeakAsyncAction> WeakThis(this);
	Subsystem->CheckHealth(
		[WeakThis](const FLocalTTSHealthResponse& Response)
		{
			if (!WeakThis.IsValid())
			{
				return;
			}

			if (Response.bOk && Response.Status.Equals(TEXT("ready"), ESearchCase::IgnoreCase))
			{
				WeakThis->Subsystem->SpeakText(
					WeakThis->SpeakRequest,
					[WeakThis](const FLocalTTSTTSResponse& TTSResponse)
					{
						if (WeakThis.IsValid())
						{
							WeakThis->HandleSpeechResponse(TTSResponse);
						}
					},
					[WeakThis](const FString& ErrorMessage)
					{
						if (WeakThis.IsValid())
						{
							WeakThis->FinishWithFailure(ErrorMessage);
						}
					});
				return;
			}

			FString ErrorMessage;
			if (!WeakThis->Subsystem->StartService(ErrorMessage))
			{
				WeakThis->FinishWithFailure(ErrorMessage);
				return;
			}

			WeakThis->BeginHealthPolling();
		},
		[WeakThis](const FString& ErrorMessage)
		{
			if (!WeakThis.IsValid())
			{
				return;
			}

			FString StartErrorMessage;
			if (!WeakThis->Subsystem->StartService(StartErrorMessage))
			{
				const FString CombinedError = StartErrorMessage.IsEmpty() ? ErrorMessage : StartErrorMessage;
				WeakThis->FinishWithFailure(CombinedError);
				return;
			}

			WeakThis->BeginHealthPolling();
		});
}

void ULocalTTSSpeakAsyncAction::BeginHealthPolling()
{
	UObject* ContextObject = WorldContextObject.Get();
	UWorld* World = ContextObject && GEngine
		? GEngine->GetWorldFromContextObject(ContextObject, EGetWorldErrorMode::ReturnNull)
		: nullptr;
	if (!World)
	{
		FinishWithFailure(TEXT("Failed to resolve world while waiting for LocalTTS service."));
		return;
	}

	const ULocalTTSSettings* Settings = GetDefault<ULocalTTSSettings>();
	const int32 MaxHealthPollCount = Settings ? Settings->MaxHealthPollCount : 30;
	const float HealthPollIntervalSeconds = Settings ? Settings->HealthPollIntervalSeconds : 1.0f;

	RemainingHealthPollCount = FMath::Max(1, MaxHealthPollCount);
	World->GetTimerManager().SetTimer(
		HealthPollTimerHandle,
		this,
		&ULocalTTSSpeakAsyncAction::PollServiceHealth,
		FMath::Max(0.1f, HealthPollIntervalSeconds),
		true);
}

void ULocalTTSSpeakAsyncAction::PollServiceHealth()
{
	if (!Subsystem.IsValid())
	{
		FinishWithFailure(TEXT("LocalTTS subsystem is unavailable while waiting for service readiness."));
		return;
	}

	if (RemainingHealthPollCount <= 0)
	{
		FinishWithFailure(TEXT("Timed out while waiting for LocalTTS service to become ready."));
		return;
	}

	--RemainingHealthPollCount;

	TWeakObjectPtr<ULocalTTSSpeakAsyncAction> WeakThis(this);
	Subsystem->CheckHealth(
		[WeakThis](const FLocalTTSHealthResponse& Response)
		{
			if (!WeakThis.IsValid())
			{
				return;
			}

			if (!Response.bOk || !Response.Status.Equals(TEXT("ready"), ESearchCase::IgnoreCase))
			{
				return;
			}

			UObject* ContextObject = WeakThis->WorldContextObject.Get();
			UWorld* World = ContextObject && GEngine
				? GEngine->GetWorldFromContextObject(ContextObject, EGetWorldErrorMode::ReturnNull)
				: nullptr;
			if (World)
			{
				World->GetTimerManager().ClearTimer(WeakThis->HealthPollTimerHandle);
			}

			WeakThis->Subsystem->SpeakText(
				WeakThis->SpeakRequest,
				[WeakThis](const FLocalTTSTTSResponse& TTSResponse)
				{
					if (WeakThis.IsValid())
					{
						WeakThis->HandleSpeechResponse(TTSResponse);
					}
				},
				[WeakThis](const FString& ErrorMessage)
				{
					if (WeakThis.IsValid())
					{
						WeakThis->FinishWithFailure(ErrorMessage);
					}
				});
		},
		[](const FString& ErrorMessage)
		{
		});
}

void ULocalTTSSpeakAsyncAction::HandleSpeechResponse(const FLocalTTSTTSResponse& Response)
{
	const FLocalTTSSpeechEvent SpeechEvent = FLocalTTSSpeechEvent::FromRequestAndResponse(SpeakRequest, Response);
	OnSpeechEventReady.Broadcast(SpeechEvent);

	if (!bAutoPlay)
	{
		OnAudioReady.Broadcast(Response);
		FinishWithSuccess(Response);
		SetReadyToDestroy();
		return;
	}

	if (!Subsystem.IsValid())
	{
		FinishWithFailure(TEXT("LocalTTS subsystem is unavailable during playback."));
		return;
	}

	TWeakObjectPtr<ULocalTTSSpeakAsyncAction> WeakThis(this);
	TFunction<void()> AudioReadyCallback = [WeakThis, Response]()
	{
		if (WeakThis.IsValid())
		{
			WeakThis->OnAudioReady.Broadcast(Response);
			WeakThis->FinishWithSuccess(Response);
		}
	};

	TFunction<void()> PlaybackFinishedCallback = [WeakThis]()
	{
		if (WeakThis.IsValid())
		{
			WeakThis->HandlePlaybackFinished();
		}
	};

	TFunction<void(const FString&)> OnFailure = [WeakThis](const FString& ErrorMessage)
	{
		if (WeakThis.IsValid())
		{
			WeakThis->FinishWithFailure(ErrorMessage);
		}
	};

	if (bUseActorPlayback)
	{
		Subsystem->PlaySpeechAtActor(
			WorldContextObject.Get(),
			Response,
			PlaybackActor.Get(),
			MoveTemp(AudioReadyCallback),
			MoveTemp(PlaybackFinishedCallback),
			MoveTemp(OnFailure));
		return;
	}

	Subsystem->PlaySpeech(
		WorldContextObject.Get(),
		Response,
		MoveTemp(AudioReadyCallback),
		MoveTemp(PlaybackFinishedCallback),
		MoveTemp(OnFailure));
}

void ULocalTTSSpeakAsyncAction::HandlePlaybackFinished()
{
	OnFinished.Broadcast();
	SetReadyToDestroy();
}

void ULocalTTSSpeakAsyncAction::FinishWithFailure(const FString& ErrorMessage)
{
	UObject* ContextObject = WorldContextObject.Get();
	UWorld* World = ContextObject && GEngine
		? GEngine->GetWorldFromContextObject(ContextObject, EGetWorldErrorMode::ReturnNull)
		: nullptr;
	if (World)
	{
		World->GetTimerManager().ClearTimer(HealthPollTimerHandle);
	}

	OnError.Broadcast(ErrorMessage);
	SetReadyToDestroy();
}

void ULocalTTSSpeakAsyncAction::FinishWithSuccess(const FLocalTTSTTSResponse& Response)
{
	UObject* ContextObject = WorldContextObject.Get();
	UWorld* World = ContextObject && GEngine
		? GEngine->GetWorldFromContextObject(ContextObject, EGetWorldErrorMode::ReturnNull)
		: nullptr;
	if (World)
	{
		World->GetTimerManager().ClearTimer(HealthPollTimerHandle);
	}

	OnSucceeded.Broadcast(Response);
}
