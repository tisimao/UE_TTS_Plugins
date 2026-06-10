// Copyright Epic Games, Inc. All Rights Reserved.

#include "ULocalTTSAudioPlayer.h"

#include "Components/SceneComponent.h"
#include "Components/AudioComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
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
		OutErrorMessage = TEXT("音频播放失败：World 上下文无效。");
		return false;
	}

	if (!Sound)
	{
		OutErrorMessage = TEXT("音频播放失败：Sound 资源无效。");
		return false;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (!World)
	{
		OutErrorMessage = TEXT("音频播放失败：无法解析当前 World。");
		return false;
	}

	if (!World->GetAudioDeviceRaw())
	{
		OutErrorMessage = TEXT("音频播放失败：当前 World 中没有可用的活动音频设备。");
		return false;
	}

	Stop();

	UAudioComponent* AudioComponent = UGameplayStatics::SpawnSound2D(World, Sound);
	if (!AudioComponent)
	{
		OutErrorMessage = TEXT("音频播放失败：无法创建 LocalTTS 的音频组件。");
		return false;
	}

	ActiveAudioComponent = AudioComponent;
	OnPlaybackFinished = MoveTemp(InOnFinished);
	bIsPaused = false;
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
		OutErrorMessage = TEXT("音频播放失败：World 上下文无效。");
		return false;
	}

	if (!Sound)
	{
		OutErrorMessage = TEXT("音频播放失败：Sound 资源无效。");
		return false;
	}

	if (!PlaybackActor)
	{
		OutErrorMessage = TEXT("3D 音频播放失败：播放目标 Actor 无效。");
		return false;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (!World)
	{
		OutErrorMessage = TEXT("音频播放失败：无法解析当前 World。");
		return false;
	}

	if (!World->GetAudioDeviceRaw())
	{
		OutErrorMessage = TEXT("音频播放失败：当前 World 中没有可用的活动音频设备。");
		return false;
	}

	Stop();

	USceneComponent* RootComponent = PlaybackActor->GetRootComponent();
	UAudioComponent* AudioComponent = RootComponent
		? UGameplayStatics::SpawnSoundAttached(Sound, RootComponent)
		: UGameplayStatics::SpawnSoundAtLocation(World, Sound, PlaybackActor->GetActorLocation());
	if (!AudioComponent)
	{
		OutErrorMessage = TEXT("3D 音频播放失败：无法创建 LocalTTS 的音频组件。");
		return false;
	}

	ActiveAudioComponent = AudioComponent;
	OnPlaybackFinished = MoveTemp(InOnFinished);
	bIsPaused = false;
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
	bIsPaused = false;
	UE_LOG(LogLocalTTSAudioPlayer, Log, TEXT("LocalTTS audio playback stopped and component released."));
}

bool ULocalTTSAudioPlayer::Pause(FString& OutErrorMessage)
{
	if (!ActiveAudioComponent || !ActiveAudioComponent->IsPlaying())
	{
		OutErrorMessage = TEXT("音频暂停失败：当前没有正在播放的 LocalTTS 音频。");
		return false;
	}

	ActiveAudioComponent->SetPaused(true);
	bIsPaused = true;
	UE_LOG(LogLocalTTSAudioPlayer, Log, TEXT("LocalTTS audio playback paused."));
	return true;
}

bool ULocalTTSAudioPlayer::Resume(FString& OutErrorMessage)
{
	if (!ActiveAudioComponent || !bIsPaused)
	{
		OutErrorMessage = TEXT("音频恢复失败：当前没有已暂停的 LocalTTS 音频。");
		return false;
	}

	ActiveAudioComponent->SetPaused(false);
	bIsPaused = false;
	UE_LOG(LogLocalTTSAudioPlayer, Log, TEXT("LocalTTS audio playback resumed."));
	return true;
}

bool ULocalTTSAudioPlayer::IsPlaying() const
{
	return ActiveAudioComponent && ActiveAudioComponent->IsPlaying();
}

bool ULocalTTSAudioPlayer::IsPaused() const
{
	return bIsPaused;
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

	bIsPaused = false;
	UE_LOG(LogLocalTTSAudioPlayer, Log, TEXT("LocalTTS audio playback finished and component released."));

	if (OnPlaybackFinished)
	{
		TFunction<void()> PlaybackFinishedCallback = MoveTemp(OnPlaybackFinished);
		PlaybackFinishedCallback();
	}
}
