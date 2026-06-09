// Copyright Epic Games, Inc. All Rights Reserved.

#include "ULocalTTSAudioPlayer.h"

#include "Components/SceneComponent.h"
#include "Components/AudioComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

DEFINE_LOG_CATEGORY_STATIC(LogLocalTTSAudioPlayer, Log, All);

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
	UE_LOG(LogLocalTTSAudioPlayer, Log, TEXT("LocalTTS 2D audio playback started."));
	return true;
}

bool ULocalTTSAudioPlayer::PlaySoundAtActor(
	UObject* WorldContextObject,
	USoundBase* Sound,
	AActor* PlaybackActor,
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

	if (!PlaybackActor)
	{
		OutErrorMessage = TEXT("Playback actor is invalid for 3D audio playback.");
		return false;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (!World)
	{
		OutErrorMessage = TEXT("Failed to resolve world for audio playback.");
		return false;
	}

	Stop();

	USceneComponent* RootComponent = PlaybackActor->GetRootComponent();
	UAudioComponent* AudioComponent = RootComponent
		? UGameplayStatics::SpawnSoundAttached(Sound, RootComponent)
		: UGameplayStatics::SpawnSoundAtLocation(World, Sound, PlaybackActor->GetActorLocation());
	if (!AudioComponent)
	{
		OutErrorMessage = TEXT("Failed to spawn 3D audio component for LocalTTS playback.");
		return false;
	}

	ActiveAudioComponent = AudioComponent;
	OnPlaybackFinished = MoveTemp(InOnFinished);
	ActiveAudioComponent->OnAudioFinishedNative.AddUObject(this, &ULocalTTSAudioPlayer::HandleAudioFinished);
	UE_LOG(
		LogLocalTTSAudioPlayer,
		Log,
		TEXT("LocalTTS 3D audio playback started actor=%s"),
		*PlaybackActor->GetName());
	return true;
}

void ULocalTTSAudioPlayer::Stop()
{
	if (!ActiveAudioComponent)
	{
		OnPlaybackFinished = nullptr;
		return;
	}

	UAudioComponent* ComponentToStop = ActiveAudioComponent;
	ActiveAudioComponent = nullptr;

	ComponentToStop->OnAudioFinishedNative.RemoveAll(this);
	ComponentToStop->Stop();
	ComponentToStop->DestroyComponent();
	ActiveAudioComponent = nullptr;
	OnPlaybackFinished = nullptr;
	UE_LOG(LogLocalTTSAudioPlayer, Log, TEXT("LocalTTS audio playback stopped and component released."));
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

	UE_LOG(LogLocalTTSAudioPlayer, Log, TEXT("LocalTTS audio playback finished and component released."));

	if (OnPlaybackFinished)
	{
		TFunction<void()> PlaybackFinishedCallback = MoveTemp(OnPlaybackFinished);
		PlaybackFinishedCallback();
	}
}
