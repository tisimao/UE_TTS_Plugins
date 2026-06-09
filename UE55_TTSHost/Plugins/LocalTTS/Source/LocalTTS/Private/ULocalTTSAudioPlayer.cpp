// Copyright Epic Games, Inc. All Rights Reserved.

#include "ULocalTTSAudioPlayer.h"

#include "Components/AudioComponent.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

bool ULocalTTSAudioPlayer::PlaySound(
	UObject* WorldContextObject,
	USoundBase* Sound,
	TFunction<void()>&& InOnFinished,
	FString& OutErrorMessage)
{
	if (!WorldContextObject || !GEngine)
	{
		OutErrorMessage = TEXT("World context is invalid for audio playback.");
		return false;
	}

	if (!Sound)
	{
		OutErrorMessage = TEXT("Sound is invalid for audio playback.");
		return false;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (!World)
	{
		OutErrorMessage = TEXT("Failed to resolve world for audio playback.");
		return false;
	}

	Stop();

	UAudioComponent* AudioComponent = UGameplayStatics::SpawnSound2D(World, Sound);
	if (!AudioComponent)
	{
		OutErrorMessage = TEXT("Failed to spawn audio component for LocalTTS playback.");
		return false;
	}

	ActiveAudioComponent = AudioComponent;
	OnPlaybackFinished = MoveTemp(InOnFinished);
	ActiveAudioComponent->OnAudioFinishedNative.AddUObject(this, &ULocalTTSAudioPlayer::HandleAudioFinished);
	return true;
}

void ULocalTTSAudioPlayer::Stop()
{
	if (!ActiveAudioComponent)
	{
		OnPlaybackFinished = nullptr;
		return;
	}

	ActiveAudioComponent->OnAudioFinishedNative.RemoveAll(this);
	ActiveAudioComponent->Stop();
	ActiveAudioComponent = nullptr;
	OnPlaybackFinished = nullptr;
}

bool ULocalTTSAudioPlayer::IsPlaying() const
{
	return ActiveAudioComponent && ActiveAudioComponent->IsPlaying();
}

void ULocalTTSAudioPlayer::HandleAudioFinished(UAudioComponent* FinishedComponent)
{
	if (FinishedComponent)
	{
		FinishedComponent->OnAudioFinishedNative.RemoveAll(this);
	}

	if (ActiveAudioComponent == FinishedComponent)
	{
		ActiveAudioComponent = nullptr;
	}

	if (OnPlaybackFinished)
	{
		TFunction<void()> PlaybackFinishedCallback = MoveTemp(OnPlaybackFinished);
		PlaybackFinishedCallback();
	}
}
