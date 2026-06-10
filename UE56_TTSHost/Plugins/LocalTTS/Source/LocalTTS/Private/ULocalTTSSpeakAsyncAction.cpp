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
		FinishWithFailure(TEXT("LocalTTS 语音请求失败：World 上下文无效。"));
		return;
	}

	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(ContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World)
	{
		FinishWithFailure(TEXT("LocalTTS 语音请求失败：无法解析当前 World。"));
		return;
	}

	UGameInstance* GameInstance = World->GetGameInstance();
	if (!GameInstance)
	{
		FinishWithFailure(TEXT("LocalTTS 语音请求失败：GameInstance 不可用。"));
		return;
	}

	RegisterWithGameInstance(GameInstance);

	ULocalTTSSubsystem* LocalSubsystem = GameInstance->GetSubsystem<ULocalTTSSubsystem>();
	if (!LocalSubsystem)
	{
		FinishWithFailure(TEXT("LocalTTS 语音请求失败：LocalTTS 子系统不可用。"));
		return;
	}

	Subsystem = LocalSubsystem;
	OnStarted.Broadcast();
	BroadcastStateChanged(ELocalTTSSpeakAsyncState::Started, TEXT("已进入 LocalTTS 语音流程。"));

	TrySendRequest();
}

void ULocalTTSSpeakAsyncAction::TrySendRequest()
{
	if (!Subsystem.IsValid())
	{
		FinishWithFailure(TEXT("LocalTTS 语音请求失败：LocalTTS 子系统不可用。"));
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
				WeakThis->BroadcastStateChanged(ELocalTTSSpeakAsyncState::Generating, TEXT("LocalTTS 服务已就绪，正在请求生成音频。"));
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

			WeakThis->BroadcastStateChanged(ELocalTTSSpeakAsyncState::WaitingForService, TEXT("LocalTTS 服务正在启动或等待模型加载。"));
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

			WeakThis->BroadcastStateChanged(ELocalTTSSpeakAsyncState::WaitingForService, TEXT("LocalTTS 服务正在启动或等待模型加载。"));
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
		FinishWithFailure(TEXT("LocalTTS 在等待服务就绪时无法解析当前 World。"));
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
		FinishWithFailure(TEXT("LocalTTS 在等待服务就绪时，LocalTTS 子系统已不可用。"));
		return;
	}

	if (RemainingHealthPollCount <= 0)
	{
		FinishWithFailure(TEXT("等待 LocalTTS 服务进入就绪状态超时。请检查服务控制台、Project Settings > Plugins > LocalTTS，以及是否已执行 Setup_TTS_Service.bat。"));
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

			WeakThis->BroadcastStateChanged(ELocalTTSSpeakAsyncState::Generating, TEXT("LocalTTS 服务已就绪，正在请求生成音频。"));
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
		BroadcastStateChanged(ELocalTTSSpeakAsyncState::AudioReady, TEXT("语音已生成完成，可用于自定义播放、数字人或后处理。"));
		OnAudioReady.Broadcast(Response);
		FinishWithSuccess(Response);
		BroadcastStateChanged(ELocalTTSSpeakAsyncState::Finished, TEXT("仅生成流程已完成。"));
		SetReadyToDestroy();
		return;
	}

	if (!Subsystem.IsValid())
	{
		FinishWithFailure(TEXT("LocalTTS 在准备播放时，LocalTTS 子系统已不可用。"));
		return;
	}

	TWeakObjectPtr<ULocalTTSSpeakAsyncAction> WeakThis(this);
	TFunction<void()> AudioReadyCallback = [WeakThis, Response]()
	{
		if (WeakThis.IsValid())
		{
			WeakThis->BroadcastStateChanged(ELocalTTSSpeakAsyncState::AudioReady, TEXT("音频已准备完成，即将开始播放。"));
			WeakThis->OnAudioReady.Broadcast(Response);
			WeakThis->BroadcastStateChanged(ELocalTTSSpeakAsyncState::Playing, TEXT("音频已开始播放。"));
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
	BroadcastStateChanged(ELocalTTSSpeakAsyncState::Finished, TEXT("音频播放结束。"));
	OnFinished.Broadcast();
	SetReadyToDestroy();
}

void ULocalTTSSpeakAsyncAction::BroadcastStateChanged(const ELocalTTSSpeakAsyncState NewState, const FString& DetailMessage)
{
	OnStateChanged.Broadcast(NewState, DetailMessage);
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

	BroadcastStateChanged(ELocalTTSSpeakAsyncState::Error, ErrorMessage);
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
