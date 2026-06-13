// Copyright Epic Games, Inc. All Rights Reserved.

#include "ULocalTTSPlayWavAsyncAction.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"

#include "ULocalTTSSubsystem.h"

ULocalTTSPlayWavAsyncAction* ULocalTTSPlayWavAsyncAction::PlayWavPathAsync(
	UObject* InWorldContextObject,
	const FString& InWavPath)
{
	ULocalTTSPlayWavAsyncAction* Action = NewObject<ULocalTTSPlayWavAsyncAction>();
	Action->WorldContextObject = InWorldContextObject;
	Action->WavPath = InWavPath;
	Action->PlaybackSource = EPlaybackSource::WavPath;
	return Action;
}

ULocalTTSPlayWavAsyncAction* ULocalTTSPlayWavAsyncAction::PlayLastWavAsync(UObject* InWorldContextObject)
{
	ULocalTTSPlayWavAsyncAction* Action = NewObject<ULocalTTSPlayWavAsyncAction>();
	Action->WorldContextObject = InWorldContextObject;
	Action->PlaybackSource = EPlaybackSource::Last;
	return Action;
}

ULocalTTSPlayWavAsyncAction* ULocalTTSPlayWavAsyncAction::PlaySpeechResponseAsync(
	UObject* InWorldContextObject,
	const FLocalTTSTTSResponse& InResponse)
{
	ULocalTTSPlayWavAsyncAction* Action = NewObject<ULocalTTSPlayWavAsyncAction>();
	Action->WorldContextObject = InWorldContextObject;
	Action->Response = InResponse;
	Action->PlaybackSource = EPlaybackSource::Response;
	return Action;
}

ULocalTTSPlayWavAsyncAction* ULocalTTSPlayWavAsyncAction::PlaySpeechEventAsync(
	UObject* InWorldContextObject,
	const FLocalTTSSpeechEvent& SpeechEvent)
{
	FLocalTTSTTSResponse PlaybackResponse;
	PlaybackResponse.bOk = SpeechEvent.bOk;
	PlaybackResponse.RequestId = SpeechEvent.RequestId;
	PlaybackResponse.Mode = SpeechEvent.Mode;
	PlaybackResponse.WavPath = SpeechEvent.WavPath;
	PlaybackResponse.SampleRate = SpeechEvent.SampleRate;
	PlaybackResponse.DurationMs = SpeechEvent.DurationMs;
	PlaybackResponse.ErrorCode = SpeechEvent.ErrorCode;
	PlaybackResponse.ErrorMessage = SpeechEvent.ErrorMessage;
	return PlaySpeechResponseAsync(InWorldContextObject, PlaybackResponse);
}

void ULocalTTSPlayWavAsyncAction::Activate()
{
	UObject* ContextObject = WorldContextObject.Get();
	if (!ContextObject)
	{
		FinishWithFailure(TEXT("LocalTTS 播放失败：World 上下文无效。"));
		return;
	}

	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(ContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World)
	{
		FinishWithFailure(TEXT("LocalTTS 播放失败：无法解析当前 World。"));
		return;
	}

	UGameInstance* GameInstance = World->GetGameInstance();
	if (!GameInstance)
	{
		FinishWithFailure(TEXT("LocalTTS 播放失败：GameInstance 不可用。"));
		return;
	}

	RegisterWithGameInstance(GameInstance);

	ULocalTTSSubsystem* Subsystem = GameInstance->GetSubsystem<ULocalTTSSubsystem>();
	if (!Subsystem)
	{
		FinishWithFailure(TEXT("LocalTTS 播放失败：LocalTTS 子系统不可用。"));
		return;
	}

	BroadcastStateChanged(ELocalTTSSpeakAsyncState::Started, TEXT("已进入 LocalTTS WAV 播放流程。"));

	TWeakObjectPtr<ULocalTTSPlayWavAsyncAction> WeakThis(this);
	TFunction<void(const FLocalTTSTTSResponse&)> AudioReadyCallback = [WeakThis](const FLocalTTSTTSResponse& ReadyResponse)
	{
		if (!WeakThis.IsValid())
		{
			return;
		}

		WeakThis->BroadcastStateChanged(ELocalTTSSpeakAsyncState::AudioReady, TEXT("WAV 已加载完成。"));
		WeakThis->OnAudioReady.Broadcast(ReadyResponse);
		WeakThis->BroadcastStateChanged(ELocalTTSSpeakAsyncState::Playing, TEXT("WAV 已开始播放。"));
	};

	TFunction<void()> FinishedCallback = [WeakThis]()
	{
		if (!WeakThis.IsValid())
		{
			return;
		}

		WeakThis->BroadcastStateChanged(ELocalTTSSpeakAsyncState::Finished, TEXT("WAV 播放结束。"));
		WeakThis->OnFinished.Broadcast();
		WeakThis->SetReadyToDestroy();
	};

	TFunction<void(const FString&)> FailureCallback = [WeakThis](const FString& ErrorMessage)
	{
		if (WeakThis.IsValid())
		{
			WeakThis->FinishWithFailure(ErrorMessage);
		}
	};

	switch (PlaybackSource)
	{
	case EPlaybackSource::Last:
		Subsystem->PlayLastSpeech(
			ContextObject,
			MoveTemp(AudioReadyCallback),
			MoveTemp(FinishedCallback),
			MoveTemp(FailureCallback));
		break;
	case EPlaybackSource::Response:
		Subsystem->PlaySpeech(
			ContextObject,
			Response,
			[WeakThis, ReadyResponse = Response]()
			{
				if (!WeakThis.IsValid())
				{
					return;
				}

				WeakThis->BroadcastStateChanged(ELocalTTSSpeakAsyncState::AudioReady, TEXT("WAV 已加载完成。"));
				WeakThis->OnAudioReady.Broadcast(ReadyResponse);
				WeakThis->BroadcastStateChanged(ELocalTTSSpeakAsyncState::Playing, TEXT("WAV 已开始播放。"));
			},
			MoveTemp(FinishedCallback),
			MoveTemp(FailureCallback));
		break;
	case EPlaybackSource::WavPath:
	default:
		Subsystem->PlayWavPath(
			ContextObject,
			WavPath,
			MoveTemp(AudioReadyCallback),
			MoveTemp(FinishedCallback),
			MoveTemp(FailureCallback));
		break;
	}
}

void ULocalTTSPlayWavAsyncAction::FinishWithFailure(const FString& ErrorMessage)
{
	BroadcastStateChanged(ELocalTTSSpeakAsyncState::Error, ErrorMessage);
	OnError.Broadcast(ErrorMessage);
	SetReadyToDestroy();
}

void ULocalTTSPlayWavAsyncAction::BroadcastStateChanged(
	const ELocalTTSSpeakAsyncState NewState,
	const FString& DetailMessage)
{
	OnStateChanged.Broadcast(NewState, DetailMessage);
}
