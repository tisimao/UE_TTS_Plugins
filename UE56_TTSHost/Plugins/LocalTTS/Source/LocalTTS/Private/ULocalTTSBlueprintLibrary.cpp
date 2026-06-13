// Copyright Epic Games, Inc. All Rights Reserved.

#include "ULocalTTSBlueprintLibrary.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"

#include "FLocalTTSRequestValidator.h"
#include "ULocalTTSLongTextQueue.h"
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
		ErrorMessage = TEXT("LocalTTS 子系统不可用。请在有效的游戏 World 中调用，确保 GameInstance 子系统已创建。");
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

bool ULocalTTSBlueprintLibrary::PauseSpeaking(const UObject* WorldContextObject, FString& ErrorMessage)
{
	ULocalTTSSubsystem* Subsystem = ResolveSubsystem(WorldContextObject);
	if (!Subsystem)
	{
		ErrorMessage = TEXT("LocalTTS 子系统不可用，无法暂停播放。");
		return false;
	}

	return Subsystem->PauseSpeaking(ErrorMessage);
}

bool ULocalTTSBlueprintLibrary::ResumeSpeaking(const UObject* WorldContextObject, FString& ErrorMessage)
{
	ULocalTTSSubsystem* Subsystem = ResolveSubsystem(WorldContextObject);
	if (!Subsystem)
	{
		ErrorMessage = TEXT("LocalTTS 子系统不可用，无法恢复播放。");
		return false;
	}

	return Subsystem->ResumeSpeaking(ErrorMessage);
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
	return Subsystem ? Subsystem->GetServiceStateText() : TEXT("LocalTTS 子系统不可用。");
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

FString ULocalTTSBlueprintLibrary::GetLocalTTSSpeakAsyncStateText(const ELocalTTSSpeakAsyncState State)
{
	switch (State)
	{
	case ELocalTTSSpeakAsyncState::Idle:
		return TEXT("空闲");
	case ELocalTTSSpeakAsyncState::Started:
		return TEXT("思考中");
	case ELocalTTSSpeakAsyncState::WaitingForService:
		return TEXT("等待服务就绪");
	case ELocalTTSSpeakAsyncState::Generating:
		return TEXT("生成中");
	case ELocalTTSSpeakAsyncState::AudioReady:
		return TEXT("音频已就绪");
	case ELocalTTSSpeakAsyncState::Playing:
		return TEXT("播放中");
	case ELocalTTSSpeakAsyncState::Finished:
		return TEXT("已完成");
	case ELocalTTSSpeakAsyncState::Error:
		return TEXT("发生错误");
	default:
		return TEXT("未知状态");
	}
}

bool ULocalTTSBlueprintLibrary::ShouldDisableSubmitForLocalTTSSpeakState(const ELocalTTSSpeakAsyncState State)
{
	switch (State)
	{
	case ELocalTTSSpeakAsyncState::Started:
	case ELocalTTSSpeakAsyncState::WaitingForService:
	case ELocalTTSSpeakAsyncState::Generating:
	case ELocalTTSSpeakAsyncState::Playing:
		return true;
	default:
		return false;
	}
}

bool ULocalTTSBlueprintLibrary::IsTerminalLocalTTSSpeakState(const ELocalTTSSpeakAsyncState State)
{
	return State == ELocalTTSSpeakAsyncState::Finished || State == ELocalTTSSpeakAsyncState::Error;
}

ULocalTTSLongTextQueue* ULocalTTSBlueprintLibrary::CreateLocalTTSLongTextQueue(UObject* WorldContextObject)
{
	return NewObject<ULocalTTSLongTextQueue>(WorldContextObject ? WorldContextObject : GetTransientPackage());
}

FString ULocalTTSBlueprintLibrary::GetLocalTTSLongTextQueueStateText(const ELocalTTSLongTextQueueState State)
{
	switch (State)
	{
	case ELocalTTSLongTextQueueState::Idle:
		return TEXT("空闲");
	case ELocalTTSLongTextQueueState::Segmenting:
		return TEXT("分段中");
	case ELocalTTSLongTextQueueState::Generating:
		return TEXT("生成中");
	case ELocalTTSLongTextQueueState::Playing:
		return TEXT("播放中");
	case ELocalTTSLongTextQueueState::Paused:
		return TEXT("已暂停");
	case ELocalTTSLongTextQueueState::Stopped:
		return TEXT("已停止");
	case ELocalTTSLongTextQueueState::Finished:
		return TEXT("已完成");
	case ELocalTTSLongTextQueueState::Error:
		return TEXT("发生错误");
	default:
		return TEXT("未知状态");
	}
}

bool ULocalTTSBlueprintLibrary::CanStartLocalTTSLongTextQueue(const ELocalTTSLongTextQueueState State)
{
	return State == ELocalTTSLongTextQueueState::Idle
		|| State == ELocalTTSLongTextQueueState::Stopped
		|| State == ELocalTTSLongTextQueueState::Finished
		|| State == ELocalTTSLongTextQueueState::Error;
}

bool ULocalTTSBlueprintLibrary::CanPauseLocalTTSLongTextQueue(const ELocalTTSLongTextQueueState State)
{
	return State == ELocalTTSLongTextQueueState::Segmenting
		|| State == ELocalTTSLongTextQueueState::Generating
		|| State == ELocalTTSLongTextQueueState::Playing;
}

bool ULocalTTSBlueprintLibrary::CanResumeLocalTTSLongTextQueue(const ELocalTTSLongTextQueueState State)
{
	return State == ELocalTTSLongTextQueueState::Paused;
}

bool ULocalTTSBlueprintLibrary::CanSkipLocalTTSLongTextQueue(const ELocalTTSLongTextQueueState State)
{
	return State == ELocalTTSLongTextQueueState::Generating
		|| State == ELocalTTSLongTextQueueState::Playing
		|| State == ELocalTTSLongTextQueueState::Paused;
}

bool ULocalTTSBlueprintLibrary::CanStopLocalTTSLongTextQueue(const ELocalTTSLongTextQueueState State)
{
	return State == ELocalTTSLongTextQueueState::Segmenting
		|| State == ELocalTTSLongTextQueueState::Generating
		|| State == ELocalTTSLongTextQueueState::Playing
		|| State == ELocalTTSLongTextQueueState::Paused;
}

bool ULocalTTSBlueprintLibrary::IsTerminalLocalTTSLongTextQueueState(const ELocalTTSLongTextQueueState State)
{
	return State == ELocalTTSLongTextQueueState::Stopped
		|| State == ELocalTTSLongTextQueueState::Finished
		|| State == ELocalTTSLongTextQueueState::Error;
}

FLocalTTSHealthResponse ULocalTTSBlueprintLibrary::GetLastLocalTTSHealth(const UObject* WorldContextObject)
{
	const ULocalTTSSubsystem* Subsystem = ResolveSubsystem(WorldContextObject);
	return Subsystem ? Subsystem->GetLastHealthResponse() : FLocalTTSHealthResponse();
}

FString ULocalTTSBlueprintLibrary::GetLastLocalTTSError(const UObject* WorldContextObject)
{
	const ULocalTTSSubsystem* Subsystem = ResolveSubsystem(WorldContextObject);
	return Subsystem ? Subsystem->GetLastHealthError() : TEXT("LocalTTS 子系统不可用。");
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

TArray<FLocalTTSTTSResponse> ULocalTTSBlueprintLibrary::GetLocalTTSSpeechHistory(const UObject* WorldContextObject)
{
	const ULocalTTSSubsystem* Subsystem = ResolveSubsystem(WorldContextObject);
	return Subsystem ? Subsystem->GetTTSResponseHistory() : TArray<FLocalTTSTTSResponse>();
}

FString ULocalTTSBlueprintLibrary::GetLastLocalTTSWavPath(const UObject* WorldContextObject)
{
	const ULocalTTSSubsystem* Subsystem = ResolveSubsystem(WorldContextObject);
	return Subsystem ? Subsystem->GetLastTTSResponse().WavPath : FString();
}

FString ULocalTTSBlueprintLibrary::GetLastLocalTTSSpeechError(const UObject* WorldContextObject)
{
	const ULocalTTSSubsystem* Subsystem = ResolveSubsystem(WorldContextObject);
	return Subsystem ? Subsystem->GetLastTTSError() : TEXT("LocalTTS 子系统不可用。");
}

ELocalTTSErrorCode ULocalTTSBlueprintLibrary::GetLastLocalTTSSpeechErrorCode(const UObject* WorldContextObject)
{
	const ULocalTTSSubsystem* Subsystem = ResolveSubsystem(WorldContextObject);
	return Subsystem ? Subsystem->GetLastTTSErrorCode() : ELocalTTSErrorCode::InternalError;
}
