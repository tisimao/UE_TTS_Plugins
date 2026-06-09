// Copyright Epic Games, Inc. All Rights Reserved.

#include "ULocalTTSCheckHealthAsyncAction.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"

#include "ULocalTTSSubsystem.h"

ULocalTTSCheckHealthAsyncAction* ULocalTTSCheckHealthAsyncAction::CheckLocalTTSHealth(UObject* WorldContextObject)
{
	ULocalTTSCheckHealthAsyncAction* Action = NewObject<ULocalTTSCheckHealthAsyncAction>();
	Action->WorldContextObject = WorldContextObject;
	return Action;
}

void ULocalTTSCheckHealthAsyncAction::Activate()
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
		FinishWithFailure(TEXT("Failed to resolve world for LocalTTS health check."));
		return;
	}

	UGameInstance* GameInstance = World->GetGameInstance();
	if (!GameInstance)
	{
		FinishWithFailure(TEXT("Game instance is unavailable."));
		return;
	}

	RegisterWithGameInstance(GameInstance);

	ULocalTTSSubsystem* Subsystem = GameInstance->GetSubsystem<ULocalTTSSubsystem>();
	if (!Subsystem)
	{
		FinishWithFailure(TEXT("LocalTTS subsystem is unavailable."));
		return;
	}

	TWeakObjectPtr<ULocalTTSCheckHealthAsyncAction> WeakThis(this);
	Subsystem->CheckHealth(
		[WeakThis](const FLocalTTSHealthResponse& Response)
		{
			if (!WeakThis.IsValid())
			{
				return;
			}

			WeakThis->OnSuccess.Broadcast(Response);
			WeakThis->SetReadyToDestroy();
		},
		[WeakThis](const FString& ErrorMessage)
		{
			if (!WeakThis.IsValid())
			{
				return;
			}

			WeakThis->OnFailure.Broadcast(ErrorMessage);
			WeakThis->SetReadyToDestroy();
		});
}

void ULocalTTSCheckHealthAsyncAction::FinishWithFailure(const FString& ErrorMessage)
{
	OnFailure.Broadcast(ErrorMessage);
	SetReadyToDestroy();
}
