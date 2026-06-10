// Copyright Epic Games, Inc. All Rights Reserved.

#include "ULocalTTSBlueprintLibrary.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"

#include "FLocalTTSRequestValidator.h"
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

FLocalTTSSpeakRequest ULocalTTSBlueprintLibrary::MakeAutoSpeakRequest(
	const FString& Text,
	const FString& LanguageId,
	float Speed)
{
	FLocalTTSSpeakRequest SpeakRequest;
	SpeakRequest.Text = Text;
	SpeakRequest.Mode = TEXT("auto");
	SpeakRequest.LanguageId = LanguageId.IsEmpty() ? TEXT("zh") : LanguageId;
	SpeakRequest.Speed = Speed <= 0.0f ? 1.0f : Speed;
	return SpeakRequest;
}

FLocalTTSSpeakRequest ULocalTTSBlueprintLibrary::MakeDesignSpeakRequest(
	const FString& Text,
	const FString& Instruct,
	const FString& LanguageId,
	float Speed)
{
	FLocalTTSSpeakRequest SpeakRequest = MakeAutoSpeakRequest(Text, LanguageId, Speed);
	SpeakRequest.Mode = TEXT("design");
	SpeakRequest.Instruct = Instruct.IsEmpty() ? TEXT("female, chinese accent") : Instruct;
	return SpeakRequest;
}

FLocalTTSSpeakRequest ULocalTTSBlueprintLibrary::MakeCloneSpeakRequest(
	const FString& Text,
	const FString& ReferenceAudioPath,
	const FString& ReferenceText,
	const FString& LanguageId,
	float Speed)
{
	FLocalTTSSpeakRequest SpeakRequest = MakeAutoSpeakRequest(Text, LanguageId, Speed);
	SpeakRequest.Mode = TEXT("clone");
	SpeakRequest.ReferenceAudioPath = ReferenceAudioPath;
	SpeakRequest.ReferenceText = ReferenceText;
	return SpeakRequest;
}

bool ULocalTTSBlueprintLibrary::ValidateSpeakRequest(
	const FLocalTTSSpeakRequest& SpeakRequest,
	FString& ErrorMessage)
{
	FLocalTTSRequestValidator Validator;
	return Validator.Validate(SpeakRequest, ErrorMessage);
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

ELocalTTSServiceState ULocalTTSBlueprintLibrary::GetLocalTTSServiceState(const UObject* WorldContextObject)
{
	const ULocalTTSSubsystem* Subsystem = ResolveSubsystem(WorldContextObject);
	return Subsystem ? Subsystem->GetServiceState() : ELocalTTSServiceState::Error;
}

FString ULocalTTSBlueprintLibrary::GetLocalTTSServiceStateText(const UObject* WorldContextObject)
{
	const ULocalTTSSubsystem* Subsystem = ResolveSubsystem(WorldContextObject);
	return Subsystem ? Subsystem->GetServiceStateText() : TEXT("LocalTTS subsystem is unavailable.");
}

bool ULocalTTSBlueprintLibrary::IsSpeaking(const UObject* WorldContextObject)
{
	const ULocalTTSSubsystem* Subsystem = ResolveSubsystem(WorldContextObject);
	return Subsystem ? Subsystem->IsSpeaking() : false;
}

bool ULocalTTSBlueprintLibrary::IsLocalTTSBusy(const UObject* WorldContextObject)
{
	const ULocalTTSSubsystem* Subsystem = ResolveSubsystem(WorldContextObject);
	return Subsystem ? Subsystem->IsBusy() : false;
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

ELocalTTSErrorCode ULocalTTSBlueprintLibrary::GetLastLocalTTSHealthErrorCode(const UObject* WorldContextObject)
{
	const ULocalTTSSubsystem* Subsystem = ResolveSubsystem(WorldContextObject);
	return Subsystem ? Subsystem->GetLastHealthErrorCode() : ELocalTTSErrorCode::InternalError;
}

FLocalTTSTTSResponse ULocalTTSBlueprintLibrary::GetLastLocalTTSSpeechResult(const UObject* WorldContextObject)
{
	const ULocalTTSSubsystem* Subsystem = ResolveSubsystem(WorldContextObject);
	return Subsystem ? Subsystem->GetLastTTSResponse() : FLocalTTSTTSResponse();
}

FString ULocalTTSBlueprintLibrary::GetLastLocalTTSWavPath(const UObject* WorldContextObject)
{
	const ULocalTTSSubsystem* Subsystem = ResolveSubsystem(WorldContextObject);
	return Subsystem ? Subsystem->GetLastTTSResponse().WavPath : FString();
}

FString ULocalTTSBlueprintLibrary::GetLastLocalTTSSpeechError(const UObject* WorldContextObject)
{
	const ULocalTTSSubsystem* Subsystem = ResolveSubsystem(WorldContextObject);
	return Subsystem ? Subsystem->GetLastTTSError() : TEXT("LocalTTS subsystem is unavailable.");
}

ELocalTTSErrorCode ULocalTTSBlueprintLibrary::GetLastLocalTTSSpeechErrorCode(const UObject* WorldContextObject)
{
	const ULocalTTSSubsystem* Subsystem = ResolveSubsystem(WorldContextObject);
	return Subsystem ? Subsystem->GetLastTTSErrorCode() : ELocalTTSErrorCode::InternalError;
}
