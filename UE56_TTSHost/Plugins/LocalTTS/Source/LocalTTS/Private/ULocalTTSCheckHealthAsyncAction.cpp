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
		FinishWithFailure(TEXT("LocalTTS 健康检查失败：World 上下文无效。"));
		return;
	}

	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(ContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World)
	{
		FinishWithFailure(TEXT("LocalTTS 健康检查失败：无法解析当前 World。"));
		return;
	}

	UGameInstance* GameInstance = World->GetGameInstance();
	if (!GameInstance)
	{
		FinishWithFailure(TEXT("LocalTTS 健康检查失败：GameInstance 不可用。"));
		return;
	}

	RegisterWithGameInstance(GameInstance);

	ULocalTTSSubsystem* Subsystem = GameInstance->GetSubsystem<ULocalTTSSubsystem>();
	if (!Subsystem)
	{
		FinishWithFailure(TEXT("LocalTTS 健康检查失败：LocalTTS 子系统不可用。"));
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
