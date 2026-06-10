// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "ULocalTTSAudioPlayer.generated.h"

class UAudioComponent;
class AActor;
class USoundBase;

UCLASS()
class LOCALTTS_API ULocalTTSAudioPlayer : public UObject
{
	GENERATED_BODY()

public:
	bool PlaySound(
		UObject* WorldContextObject,
		USoundBase* Sound,
		TFunction<void()>&& OnFinished,
		FString& OutErrorMessage);

	bool PlaySoundAtActor(
		UObject* WorldContextObject,
		USoundBase* Sound,
		AActor* PlaybackActor,
		TFunction<void()>&& OnFinished,
		FString& OutErrorMessage);

	void Stop();
	bool Pause(FString& OutErrorMessage);
	bool Resume(FString& OutErrorMessage);
	bool IsPlaying() const;
	bool IsPaused() const;

private:
	void HandleAudioFinished(UAudioComponent* FinishedComponent);

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> ActiveAudioComponent;

	TFunction<void()> OnPlaybackFinished;
	bool bIsPaused = false;
};
