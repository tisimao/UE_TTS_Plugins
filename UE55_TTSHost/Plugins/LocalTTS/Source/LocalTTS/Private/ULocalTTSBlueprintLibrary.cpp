// Copyright Epic Games, Inc. All Rights Reserved.

#include "ULocalTTSBlueprintLibrary.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"

#include "ULocalTTSSubsystem.h"

ULocalTTSSubsystem* ULocalTTSBlueprintLibrary::ResolveSubsystem(const UObject* WorldContextObject)
{
	if (!WorldContextObject || !GEngine)
	{
		return nullptr;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (!World)
	{
		return nullptr;
	}

	UGameInstance* GameInstance = World->GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<ULocalTTSSubsystem>() : nullptr;
}

bool ULocalTTSBlueprintLibrary::StartLocalTTS(const UObject* WorldContextObject, FString& ErrorMessage)
{
	ULocalTTSSubsystem* Subsystem = ResolveSubsystem(WorldContextObject);
	if (!Subsystem)
	{
		ErrorMessage = TEXT("LocalTTS subsystem is unavailable.");
		return false;
	}

	return Subsystem->StartService(ErrorMessage);
}

void ULocalTTSBlueprintLibrary::StopLocalTTS(const UObject* WorldContextObject)
{
	ULocalTTSSubsystem* Subsystem = ResolveSubsystem(WorldContextObject);
	if (Subsystem)
	{
		Subsystem->StopService();
	}
}

void ULocalTTSBlueprintLibrary::StopSpeaking(const UObject* WorldContextObject)
{
	ULocalTTSSubsystem* Subsystem = ResolveSubsystem(WorldContextObject);
	if (Subsystem)
	{
		Subsystem->StopSpeaking();
	}
}

bool ULocalTTSBlueprintLibrary::IsLocalTTSReady(const UObject* WorldContextObject)
{
	const ULocalTTSSubsystem* Subsystem = ResolveSubsystem(WorldContextObject);
	return Subsystem ? Subsystem->IsServiceReady() : false;
}

bool ULocalTTSBlueprintLibrary::IsSpeaking(const UObject* WorldContextObject)
{
	const ULocalTTSSubsystem* Subsystem = ResolveSubsystem(WorldContextObject);
	return Subsystem ? Subsystem->IsSpeaking() : false;
}

FLocalTTSHealthResponse ULocalTTSBlueprintLibrary::GetLastLocalTTSHealth(const UObject* WorldContextObject)
{
	const ULocalTTSSubsystem* Subsystem = ResolveSubsystem(WorldContextObject);
	return Subsystem ? Subsystem->GetLastHealthResponse() : FLocalTTSHealthResponse();
}

FString ULocalTTSBlueprintLibrary::GetLastLocalTTSError(const UObject* WorldContextObject)
{
	const ULocalTTSSubsystem* Subsystem = ResolveSubsystem(WorldContextObject);
	return Subsystem ? Subsystem->GetLastHealthError() : TEXT("LocalTTS subsystem is unavailable.");
}

FLocalTTSTTSResponse ULocalTTSBlueprintLibrary::GetLastLocalTTSSpeechResult(const UObject* WorldContextObject)
{
	const ULocalTTSSubsystem* Subsystem = ResolveSubsystem(WorldContextObject);
	return Subsystem ? Subsystem->GetLastTTSResponse() : FLocalTTSTTSResponse();
}

FString ULocalTTSBlueprintLibrary::GetLastLocalTTSSpeechError(const UObject* WorldContextObject)
{
	const ULocalTTSSubsystem* Subsystem = ResolveSubsystem(WorldContextObject);
	return Subsystem ? Subsystem->GetLastTTSError() : TEXT("LocalTTS subsystem is unavailable.");
}
