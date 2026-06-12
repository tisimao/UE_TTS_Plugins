// Copyright Epic Games, Inc. All Rights Reserved.

#include "ALocalTTSDemoController.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"

#include "ULocalTTSBlueprintLibrary.h"
#include "ULocalTTSLongTextQueue.h"
#include "ULocalTTSSettings.h"
#include "ULocalTTSSpeakAsyncAction.h"
#include "ULocalTTSSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogLocalTTSDemoController, Log, All);

namespace LocalTTSDemoControllerText
{
	static const TCHAR* NoneText()
	{
		return TEXT("无");
	}

	static const TCHAR* PlayModeOnlyMessage()
	{
		return TEXT("LocalTTS Demo 控制器需要在 Play/PIE 模式下使用，因为它依赖 GameInstance 中的 LocalTTS 子系统。");
	}

	static const TCHAR* EmptySingleTextMessage()
	{
		return TEXT("单句文本为空。请在 UI 输入要朗读的内容，或在 Demo 控制器上填写默认单句文本。");
	}

	static const TCHAR* EmptyLongTextMessage()
	{
		return TEXT("长文本为空。请在 UI 输入要分段朗读的内容，或在 Demo 控制器上填写默认长文本。");
	}

	static FString ToYesNo(const bool bValue)
	{
		return bValue ? TEXT("是") : TEXT("否");
	}
}

ALocalTTSDemoController::ALocalTTSDemoController()
{
	PrimaryActorTick.bCanEverTick = false;

	DefaultSingleText = TEXT("你好，这是 LocalTTS Demo 的单句语音测试。");
	DefaultLongText = TEXT("这是 LocalTTS Demo 的长文本测试。第一段会先生成语音并播放。第二段用于验证暂停、继续和跳段按钮。第三段用于观察队列完成后的状态回调。");

	SingleSpeakRequestTemplate.Mode = TEXT("auto");
	SingleSpeakRequestTemplate.LanguageId = TEXT("zh");
	SingleSpeakRequestTemplate.Speed = 1.0f;

	LongTextRequestTemplate.SourceText = DefaultLongText;
	LongTextRequestTemplate.SpeakRequestTemplate = SingleSpeakRequestTemplate;
	LongTextRequestTemplate.MaxCharactersPerSegment = 80;
	LongTextRequestTemplate.MinCharactersPerSegment = 12;
	LongTextRequestTemplate.bSplitOnNewLine = true;
	LongTextRequestTemplate.bUseRecommendedDuration = false;

	RefreshStateTexts();
}

void ALocalTTSDemoController::BeginPlay()
{
	Super::BeginPlay();

	CreateOrResetLongTextQueue();
	RefreshStateTexts();
	AppendEventLog(TEXT("Demo 控制器已初始化。"));
}

bool ALocalTTSDemoController::StartService(FString& ErrorMessage)
{
	ErrorMessage.Reset();

	ULocalTTSSubsystem* Subsystem = ResolveSubsystem();
	if (!Subsystem)
	{
		ErrorMessage = LocalTTSDemoControllerText::PlayModeOnlyMessage();
		StoreError(ErrorMessage);
		return false;
	}

	if (!Subsystem->StartService(ErrorMessage))
	{
		StoreError(ErrorMessage);
		return false;
	}

	ClearError();
	ServiceStateText = Subsystem->GetServiceStateText();
	AppendEventLog(TEXT("已发送服务启动请求，等待健康检查确认模型就绪。"));
	BeginServiceReadinessPolling();
	BroadcastStateUpdated();
	return true;
}

bool ALocalTTSDemoController::CheckHealth(FString& ErrorMessage)
{
	ErrorMessage.Reset();

	ULocalTTSSubsystem* Subsystem = ResolveSubsystem();
	if (!Subsystem)
	{
		ErrorMessage = LocalTTSDemoControllerText::PlayModeOnlyMessage();
		StoreError(ErrorMessage);
		return false;
	}

	ServiceStateText = TEXT("健康检查中...");
	AppendEventLog(TEXT("开始检查 LocalTTS 健康状态。"));
	BroadcastStateUpdated();

	TWeakObjectPtr<ALocalTTSDemoController> WeakThis(this);
	Subsystem->CheckHealth(
		[WeakThis](const FLocalTTSHealthResponse& Response)
		{
			if (!WeakThis.IsValid())
			{
				return;
			}

			WeakThis->LastHealthResponse = Response;
			WeakThis->ClearError();

			if (ULocalTTSSubsystem* CurrentSubsystem = WeakThis->ResolveSubsystem())
			{
				WeakThis->ServiceStateText = CurrentSubsystem->GetServiceStateText();
			}

			WeakThis->AppendEventLog(FString::Printf(
				TEXT("健康检查完成：成功=%s，状态=%s，模型=%s。"),
				*LocalTTSDemoControllerText::ToYesNo(Response.bOk),
				*Response.Status,
				*Response.Model));
			if (Response.bOk && Response.Status.Equals(TEXT("ready"), ESearchCase::IgnoreCase))
			{
				WeakThis->StopServiceReadinessPolling();
				WeakThis->AppendEventLog(TEXT("LocalTTS 服务已就绪，可以开始生成语音。"));
			}
			WeakThis->BroadcastStateUpdated();
		},
		[WeakThis](const FString& FailureMessage)
		{
			if (!WeakThis.IsValid())
			{
				return;
			}

			WeakThis->StoreError(FailureMessage);
			if (ULocalTTSSubsystem* CurrentSubsystem = WeakThis->ResolveSubsystem())
			{
				WeakThis->ServiceStateText = CurrentSubsystem->GetServiceStateText();
			}
			WeakThis->AppendEventLog(TEXT("健康检查失败。"));
			WeakThis->BroadcastStateUpdated();
		});

	return true;
}

bool ALocalTTSDemoController::SpeakSingle(const FString& Text, FString& ErrorMessage)
{
	return StartSingleInternal(Text, true, ErrorMessage);
}

bool ALocalTTSDemoController::GenerateSingle(const FString& Text, FString& ErrorMessage)
{
	return StartSingleInternal(Text, false, ErrorMessage);
}

void ALocalTTSDemoController::StopSingle()
{
	if (ULocalTTSSubsystem* Subsystem = ResolveSubsystem())
	{
		Subsystem->StopSpeaking();
	}

	UpdateSingleState(ELocalTTSSpeakAsyncState::Finished, TEXT("已请求停止当前播放。生成中的服务端请求会在返回后被忽略或进入结束状态。"));
	AppendEventLog(TEXT("已请求停止单句播放。"));
	BroadcastStateUpdated();
}

bool ALocalTTSDemoController::PauseSingle(FString& ErrorMessage)
{
	ErrorMessage.Reset();

	ULocalTTSSubsystem* Subsystem = ResolveSubsystem();
	if (!Subsystem)
	{
		ErrorMessage = LocalTTSDemoControllerText::PlayModeOnlyMessage();
		StoreError(ErrorMessage);
		return false;
	}

	if (!Subsystem->PauseSpeaking(ErrorMessage))
	{
		StoreError(ErrorMessage);
		return false;
	}

	ClearError();
	AppendEventLog(TEXT("已暂停单句播放。"));
	BroadcastStateUpdated();
	return true;
}

bool ALocalTTSDemoController::ResumeSingle(FString& ErrorMessage)
{
	ErrorMessage.Reset();

	ULocalTTSSubsystem* Subsystem = ResolveSubsystem();
	if (!Subsystem)
	{
		ErrorMessage = LocalTTSDemoControllerText::PlayModeOnlyMessage();
		StoreError(ErrorMessage);
		return false;
	}

	if (!Subsystem->ResumeSpeaking(ErrorMessage))
	{
		StoreError(ErrorMessage);
		return false;
	}

	ClearError();
	AppendEventLog(TEXT("已继续单句播放。"));
	BroadcastStateUpdated();
	return true;
}

ULocalTTSLongTextQueue* ALocalTTSDemoController::CreateOrResetLongTextQueue()
{
	if (LongTextQueue)
	{
		LongTextQueue->StopQueue();
		UnbindLongTextQueueCallbacks();
	}

	LongTextQueue = ULocalTTSBlueprintLibrary::CreateLocalTTSLongTextQueue(this);
	BindLongTextQueueCallbacks();

	CurrentSegmentIndex = INDEX_NONE;
	TotalSegmentCount = 0;
	CurrentSegmentText.Reset();
	UpdateQueueState(ELocalTTSLongTextQueueState::Idle, TEXT("长文本队列已创建。"));
	AppendEventLog(TEXT("长文本队列已创建或重置。"));
	BroadcastStateUpdated();
	return LongTextQueue;
}

bool ALocalTTSDemoController::StartLongTextSpeak(const FString& Text, FString& ErrorMessage)
{
	return StartLongTextInternal(Text, true, ErrorMessage);
}

bool ALocalTTSDemoController::StartLongTextGenerate(const FString& Text, FString& ErrorMessage)
{
	return StartLongTextInternal(Text, false, ErrorMessage);
}

bool ALocalTTSDemoController::PauseLongText(FString& ErrorMessage)
{
	ErrorMessage.Reset();

	if (!LongTextQueue)
	{
		CreateOrResetLongTextQueue();
	}

	if (!LongTextQueue || !LongTextQueue->PauseQueue(ErrorMessage))
	{
		if (ErrorMessage.IsEmpty())
		{
			ErrorMessage = TEXT("长文本队列不可用，无法暂停。");
		}
		StoreError(ErrorMessage);
		return false;
	}

	ClearError();
	AppendEventLog(TEXT("已请求暂停长文本队列。"));
	BroadcastStateUpdated();
	return true;
}

bool ALocalTTSDemoController::ResumeLongText(FString& ErrorMessage)
{
	ErrorMessage.Reset();

	if (!LongTextQueue || !LongTextQueue->ResumeQueue(ErrorMessage))
	{
		if (ErrorMessage.IsEmpty())
		{
			ErrorMessage = TEXT("长文本队列不可用，无法继续。");
		}
		StoreError(ErrorMessage);
		return false;
	}

	ClearError();
	AppendEventLog(TEXT("已请求继续长文本队列。"));
	BroadcastStateUpdated();
	return true;
}

bool ALocalTTSDemoController::SkipLongText(FString& ErrorMessage)
{
	ErrorMessage.Reset();

	if (!LongTextQueue || !LongTextQueue->SkipToNext(ErrorMessage))
	{
		if (ErrorMessage.IsEmpty())
		{
			ErrorMessage = TEXT("长文本队列不可用，无法跳段。");
		}
		StoreError(ErrorMessage);
		return false;
	}

	ClearError();
	AppendEventLog(TEXT("已请求跳到下一段。"));
	BroadcastStateUpdated();
	return true;
}

void ALocalTTSDemoController::StopLongText()
{
	if (LongTextQueue)
	{
		LongTextQueue->StopQueue();
	}

	AppendEventLog(TEXT("已请求停止长文本队列。"));
	BroadcastStateUpdated();
}

void ALocalTTSDemoController::RefreshStateTexts()
{
	if (ULocalTTSSubsystem* Subsystem = ResolveSubsystem())
	{
		ServiceStateText = Subsystem->GetServiceStateText();
	}
	else if (ServiceStateText.IsEmpty())
	{
		ServiceStateText = LocalTTSDemoControllerText::NoneText();
	}

	SingleStateText = ULocalTTSBlueprintLibrary::GetLocalTTSSpeakAsyncStateText(SingleState);
	QueueStateText = ULocalTTSBlueprintLibrary::GetLocalTTSLongTextQueueStateText(QueueState);
	BroadcastStateUpdated();
}

void ALocalTTSDemoController::ClearEventLog()
{
	EventLogLines.Reset();
	EventLogText.Reset();
	BroadcastStateUpdated();
}

FString ALocalTTSDemoController::GetLongTextProgressText() const
{
	if (TotalSegmentCount <= 0 || CurrentSegmentIndex < 0)
	{
		return TEXT("0 / 0");
	}

	return FString::Printf(TEXT("%d / %d"), CurrentSegmentIndex + 1, TotalSegmentCount);
}

void ALocalTTSDemoController::HandleSingleStarted()
{
	AppendEventLog(TEXT("单句流程已开始。"));
	BroadcastStateUpdated();
}

void ALocalTTSDemoController::HandleSingleSucceeded(const FLocalTTSTTSResponse& Response)
{
	LastTTSResponse = Response;
	LastWavPath = Response.WavPath;
	LastRequestId = Response.RequestId;
	ClearError();
	AppendEventLog(FString::Printf(TEXT("单句生成成功：请求ID=%s。"), *Response.RequestId));
	BroadcastStateUpdated();
}

void ALocalTTSDemoController::HandleSingleAudioReady(const FLocalTTSTTSResponse& Response)
{
	LastTTSResponse = Response;
	LastWavPath = Response.WavPath;
	LastRequestId = Response.RequestId;
	AppendEventLog(FString::Printf(TEXT("单句音频已准备：%s。"), *Response.WavPath));
	BroadcastStateUpdated();
}

void ALocalTTSDemoController::HandleSingleSpeechEventReady(const FLocalTTSSpeechEvent& SpeechEvent)
{
	LastSpeechEvent = SpeechEvent;
	LastWavPath = SpeechEvent.WavPath;
	LastRequestId = SpeechEvent.RequestId;
	AppendEventLog(TEXT("单句语音事件已准备，可用于字幕、数字人或调试显示。"));
	BroadcastStateUpdated();
}

void ALocalTTSDemoController::HandleSingleFinished()
{
	if (ULocalTTSSubsystem* Subsystem = ResolveSubsystem())
	{
		Subsystem->StopSpeaking();
		ServiceStateText = Subsystem->GetServiceStateText();
	}

	UpdateSingleState(ELocalTTSSpeakAsyncState::Finished, TEXT("单句语音流程已完成。"));
	ClearActiveSingleAction();
	AppendEventLog(TEXT("单句流程结束。"));
	BroadcastStateUpdated();
}

void ALocalTTSDemoController::HandleSingleError(const FString& ErrorMessage)
{
	StoreError(ErrorMessage);
	UpdateSingleState(ELocalTTSSpeakAsyncState::Error, ErrorMessage);
	ClearActiveSingleAction();
	BroadcastStateUpdated();
}

void ALocalTTSDemoController::HandleSingleStateChanged(const ELocalTTSSpeakAsyncState State, const FString& DetailMessage)
{
	UpdateSingleState(State, DetailMessage);
	if (State == ELocalTTSSpeakAsyncState::Finished && !bActiveSingleAutoPlay)
	{
		ClearActiveSingleAction();
		AppendEventLog(TEXT("单句仅生成流程结束。"));
		BroadcastStateUpdated();
	}
}

void ALocalTTSDemoController::HandleQueueStarted()
{
	AppendEventLog(TEXT("长文本队列已开始。"));
	BroadcastStateUpdated();
}

void ALocalTTSDemoController::HandleQueueStateChanged(const ELocalTTSLongTextQueueState State, const FString& DetailMessage)
{
	UpdateQueueState(State, DetailMessage);
}

void ALocalTTSDemoController::HandleSegmentStarted(const FLocalTTSTextSegment& Segment)
{
	CurrentSegmentIndex = Segment.SegmentIndex;
	CurrentSegmentText = Segment.Text;
	if (LongTextQueue)
	{
		TotalSegmentCount = LongTextQueue->GetSegments().Num();
	}
	AppendEventLog(FString::Printf(TEXT("开始处理第 %d 段。"), CurrentSegmentIndex + 1));
	BroadcastStateUpdated();
}

void ALocalTTSDemoController::HandleSegmentGenerated(const FLocalTTSSegmentSpeechEvent& SegmentSpeechEvent)
{
	LastSegmentSpeechEvent = SegmentSpeechEvent;
	LastSpeechEvent = SegmentSpeechEvent.SpeechEvent;
	LastWavPath = SegmentSpeechEvent.SpeechEvent.WavPath;
	LastRequestId = SegmentSpeechEvent.SpeechEvent.RequestId;
	AppendEventLog(FString::Printf(TEXT("第 %d 段已生成 WAV。"), SegmentSpeechEvent.Segment.SegmentIndex + 1));
	BroadcastStateUpdated();
}

void ALocalTTSDemoController::HandleSegmentFinished(const FLocalTTSSegmentSpeechEvent& SegmentSpeechEvent)
{
	LastSegmentSpeechEvent = SegmentSpeechEvent;
	LastSpeechEvent = SegmentSpeechEvent.SpeechEvent;
	LastWavPath = SegmentSpeechEvent.SpeechEvent.WavPath;
	LastRequestId = SegmentSpeechEvent.SpeechEvent.RequestId;
	AppendEventLog(FString::Printf(TEXT("第 %d 段处理完成。"), SegmentSpeechEvent.Segment.SegmentIndex + 1));
	BroadcastStateUpdated();
}

void ALocalTTSDemoController::HandleQueueFinished()
{
	UpdateQueueState(ELocalTTSLongTextQueueState::Finished, TEXT("长文本队列已全部完成。"));
	AppendEventLog(TEXT("长文本队列已全部完成。"));
	BroadcastStateUpdated();
}

void ALocalTTSDemoController::HandleQueueStopped()
{
	UpdateQueueState(ELocalTTSLongTextQueueState::Stopped, TEXT("长文本队列已停止。"));
	AppendEventLog(TEXT("长文本队列已停止。"));
	BroadcastStateUpdated();
}

void ALocalTTSDemoController::HandleQueueError(const int32 SegmentIndex, const FString& SegmentText, const FString& ErrorMessage)
{
	CurrentSegmentIndex = SegmentIndex;
	CurrentSegmentText = SegmentText;
	StoreError(ErrorMessage);
	UpdateQueueState(ELocalTTSLongTextQueueState::Error, ErrorMessage);
	BroadcastStateUpdated();
}

ULocalTTSSubsystem* ALocalTTSDemoController::ResolveSubsystem() const
{
	const UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return nullptr;
	}

	UGameInstance* GameInstance = World->GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<ULocalTTSSubsystem>() : nullptr;
}

FLocalTTSSpeakRequest ALocalTTSDemoController::BuildSingleRequest(const FString& Text) const
{
	FLocalTTSSpeakRequest Request = SingleSpeakRequestTemplate;
	Request.Text = Text.IsEmpty() ? DefaultSingleText : Text;

	if (Request.Mode.IsEmpty())
	{
		Request.Mode = TEXT("auto");
	}

	if (Request.LanguageId.IsEmpty())
	{
		Request.LanguageId = TEXT("zh");
	}

	if (Request.Speed <= 0.0f)
	{
		Request.Speed = 1.0f;
	}

	return Request;
}

FLocalTTSLongTextRequest ALocalTTSDemoController::BuildLongTextRequest(const FString& Text) const
{
	FLocalTTSLongTextRequest Request = LongTextRequestTemplate;
	Request.SourceText = Text.IsEmpty() ? DefaultLongText : Text;

	if (Request.SpeakRequestTemplate.Mode.IsEmpty())
	{
		Request.SpeakRequestTemplate.Mode = TEXT("auto");
	}

	if (Request.SpeakRequestTemplate.LanguageId.IsEmpty())
	{
		Request.SpeakRequestTemplate.LanguageId = TEXT("zh");
	}

	if (Request.SpeakRequestTemplate.Speed <= 0.0f)
	{
		Request.SpeakRequestTemplate.Speed = 1.0f;
	}

	return Request;
}

bool ALocalTTSDemoController::StartSingleInternal(const FString& Text, const bool bAutoPlay, FString& ErrorMessage)
{
	ErrorMessage.Reset();

	const FLocalTTSSpeakRequest Request = BuildSingleRequest(Text);
	if (Request.Text.TrimStartAndEnd().IsEmpty())
	{
		ErrorMessage = LocalTTSDemoControllerText::EmptySingleTextMessage();
		StoreError(ErrorMessage);
		return false;
	}

	ULocalTTSSubsystem* Subsystem = ResolveSubsystem();
	if (!Subsystem)
	{
		ErrorMessage = LocalTTSDemoControllerText::PlayModeOnlyMessage();
		StoreError(ErrorMessage);
		return false;
	}

	if (!ULocalTTSBlueprintLibrary::ValidateSpeakRequest(Request, ErrorMessage))
	{
		StoreError(ErrorMessage);
		return false;
	}

	ClearActiveSingleAction();

	ActiveSingleAction = bAutoPlay
		? ULocalTTSSpeakAsyncAction::SpeakTextAsync(this, Request)
		: ULocalTTSSpeakAsyncAction::GenerateSpeechAsync(this, Request);

	if (!ActiveSingleAction)
	{
		ErrorMessage = TEXT("创建 LocalTTS 单句异步动作失败。");
		StoreError(ErrorMessage);
		return false;
	}

	ActiveSingleAction->OnStarted.AddDynamic(this, &ALocalTTSDemoController::HandleSingleStarted);
	ActiveSingleAction->OnSucceeded.AddDynamic(this, &ALocalTTSDemoController::HandleSingleSucceeded);
	ActiveSingleAction->OnAudioReady.AddDynamic(this, &ALocalTTSDemoController::HandleSingleAudioReady);
	ActiveSingleAction->OnSpeechEventReady.AddDynamic(this, &ALocalTTSDemoController::HandleSingleSpeechEventReady);
	ActiveSingleAction->OnFinished.AddDynamic(this, &ALocalTTSDemoController::HandleSingleFinished);
	ActiveSingleAction->OnError.AddDynamic(this, &ALocalTTSDemoController::HandleSingleError);
	ActiveSingleAction->OnStateChanged.AddDynamic(this, &ALocalTTSDemoController::HandleSingleStateChanged);
	bActiveSingleAutoPlay = bAutoPlay;

	ClearError();
	UpdateSingleState(ELocalTTSSpeakAsyncState::Started, bAutoPlay ? TEXT("开始单句生成并播放。") : TEXT("开始单句仅生成。"));
	AppendEventLog(bAutoPlay ? TEXT("单句生成并播放请求已提交。") : TEXT("单句仅生成请求已提交。"));
	ActiveSingleAction->Activate();
	BroadcastStateUpdated();
	return true;
}

bool ALocalTTSDemoController::StartLongTextInternal(const FString& Text, const bool bAutoPlay, FString& ErrorMessage)
{
	ErrorMessage.Reset();

	if (!LongTextQueue)
	{
		CreateOrResetLongTextQueue();
	}

	if (!LongTextQueue)
	{
		ErrorMessage = TEXT("创建长文本队列失败。");
		StoreError(ErrorMessage);
		return false;
	}

	const FLocalTTSLongTextRequest Request = BuildLongTextRequest(Text);
	if (Request.SourceText.TrimStartAndEnd().IsEmpty())
	{
		ErrorMessage = LocalTTSDemoControllerText::EmptyLongTextMessage();
		StoreError(ErrorMessage);
		return false;
	}

	const bool bStarted = bAutoPlay
		? LongTextQueue->StartSpeakQueue(this, Request, ErrorMessage)
		: LongTextQueue->StartGenerateQueue(this, Request, ErrorMessage);

	if (!bStarted)
	{
		StoreError(ErrorMessage);
		return false;
	}

	TotalSegmentCount = LongTextQueue->GetSegments().Num();
	CurrentSegmentIndex = LongTextQueue->GetCurrentSegmentIndex();
	CurrentSegmentText.Reset();
	ClearError();
	AppendEventLog(bAutoPlay ? TEXT("长文本生成并播放请求已提交。") : TEXT("长文本仅生成请求已提交。"));
	BroadcastStateUpdated();
	return true;
}

void ALocalTTSDemoController::BeginServiceReadinessPolling()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		StoreError(TEXT("启动服务后无法解析当前 World，不能自动检查健康状态。"));
		return;
	}

	const ULocalTTSSettings* Settings = GetDefault<ULocalTTSSettings>();
	const int32 MaxHealthPollCount = Settings ? Settings->MaxHealthPollCount : 30;
	const float HealthPollIntervalSeconds = Settings ? Settings->HealthPollIntervalSeconds : 1.0f;

	RemainingServiceReadinessPollCount = FMath::Max(1, MaxHealthPollCount);
	World->GetTimerManager().ClearTimer(ServiceReadinessTimerHandle);
	World->GetTimerManager().SetTimer(
		ServiceReadinessTimerHandle,
		this,
		&ALocalTTSDemoController::PollServiceReadiness,
		FMath::Max(0.1f, HealthPollIntervalSeconds),
		true,
		0.1f);
}

void ALocalTTSDemoController::PollServiceReadiness()
{
	if (bServiceReadinessCheckInFlight)
	{
		return;
	}

	ULocalTTSSubsystem* Subsystem = ResolveSubsystem();
	if (!Subsystem)
	{
		StopServiceReadinessPolling();
		StoreError(TEXT("启动服务后自动检查健康状态时，LocalTTS 子系统已不可用。"));
		return;
	}

	if (RemainingServiceReadinessPollCount <= 0)
	{
		StopServiceReadinessPolling();
		if (!Subsystem->IsServiceReady())
		{
			StoreError(TEXT("等待 LocalTTS 服务进入就绪状态超时。请检查服务控制台、Project Settings > Plugins > LocalTTS，以及是否已执行 Setup_TTS_Service.bat。"));
		}
		return;
	}

	--RemainingServiceReadinessPollCount;
	bServiceReadinessCheckInFlight = true;
	ServiceStateText = TEXT("健康检查中...");
	BroadcastStateUpdated();

	TWeakObjectPtr<ALocalTTSDemoController> WeakThis(this);
	Subsystem->CheckHealth(
		[WeakThis](const FLocalTTSHealthResponse& Response)
		{
			if (!WeakThis.IsValid())
			{
				return;
			}

			WeakThis->bServiceReadinessCheckInFlight = false;
			WeakThis->LastHealthResponse = Response;
			if (ULocalTTSSubsystem* CurrentSubsystem = WeakThis->ResolveSubsystem())
			{
				WeakThis->ServiceStateText = CurrentSubsystem->GetServiceStateText();
			}

			WeakThis->AppendEventLog(FString::Printf(
				TEXT("自动健康检查：成功=%s，状态=%s，模型=%s。"),
				*LocalTTSDemoControllerText::ToYesNo(Response.bOk),
				*Response.Status,
				*Response.Model));

			if (Response.bOk && Response.Status.Equals(TEXT("ready"), ESearchCase::IgnoreCase))
			{
				WeakThis->ClearError();
				WeakThis->StopServiceReadinessPolling();
				WeakThis->AppendEventLog(TEXT("LocalTTS 服务已就绪，可以开始生成语音。"));
			}

			WeakThis->BroadcastStateUpdated();
		},
		[WeakThis](const FString& FailureMessage)
		{
			if (!WeakThis.IsValid())
			{
				return;
			}

			WeakThis->bServiceReadinessCheckInFlight = false;
			if (ULocalTTSSubsystem* CurrentSubsystem = WeakThis->ResolveSubsystem())
			{
				WeakThis->ServiceStateText = CurrentSubsystem->GetServiceStateText();
			}

			WeakThis->AppendEventLog(FString::Printf(TEXT("自动健康检查暂未就绪：%s"), *FailureMessage));
			WeakThis->BroadcastStateUpdated();
		});
}

void ALocalTTSDemoController::StopServiceReadinessPolling()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ServiceReadinessTimerHandle);
	}
	RemainingServiceReadinessPollCount = 0;
	bServiceReadinessCheckInFlight = false;
}

void ALocalTTSDemoController::BindLongTextQueueCallbacks()
{
	if (!LongTextQueue)
	{
		return;
	}

	LongTextQueue->OnQueueStarted.AddDynamic(this, &ALocalTTSDemoController::HandleQueueStarted);
	LongTextQueue->OnQueueStateChanged.AddDynamic(this, &ALocalTTSDemoController::HandleQueueStateChanged);
	LongTextQueue->OnSegmentStarted.AddDynamic(this, &ALocalTTSDemoController::HandleSegmentStarted);
	LongTextQueue->OnSegmentGenerated.AddDynamic(this, &ALocalTTSDemoController::HandleSegmentGenerated);
	LongTextQueue->OnSegmentFinished.AddDynamic(this, &ALocalTTSDemoController::HandleSegmentFinished);
	LongTextQueue->OnQueueFinished.AddDynamic(this, &ALocalTTSDemoController::HandleQueueFinished);
	LongTextQueue->OnQueueStopped.AddDynamic(this, &ALocalTTSDemoController::HandleQueueStopped);
	LongTextQueue->OnQueueError.AddDynamic(this, &ALocalTTSDemoController::HandleQueueError);
}

void ALocalTTSDemoController::UnbindLongTextQueueCallbacks()
{
	if (!LongTextQueue)
	{
		return;
	}

	LongTextQueue->OnQueueStarted.RemoveAll(this);
	LongTextQueue->OnQueueStateChanged.RemoveAll(this);
	LongTextQueue->OnSegmentStarted.RemoveAll(this);
	LongTextQueue->OnSegmentGenerated.RemoveAll(this);
	LongTextQueue->OnSegmentFinished.RemoveAll(this);
	LongTextQueue->OnQueueFinished.RemoveAll(this);
	LongTextQueue->OnQueueStopped.RemoveAll(this);
	LongTextQueue->OnQueueError.RemoveAll(this);
}

void ALocalTTSDemoController::ClearActiveSingleAction()
{
	if (!ActiveSingleAction)
	{
		bActiveSingleAutoPlay = false;
		return;
	}

	ActiveSingleAction->OnStarted.RemoveAll(this);
	ActiveSingleAction->OnSucceeded.RemoveAll(this);
	ActiveSingleAction->OnAudioReady.RemoveAll(this);
	ActiveSingleAction->OnSpeechEventReady.RemoveAll(this);
	ActiveSingleAction->OnFinished.RemoveAll(this);
	ActiveSingleAction->OnError.RemoveAll(this);
	ActiveSingleAction->OnStateChanged.RemoveAll(this);
	ActiveSingleAction = nullptr;
	bActiveSingleAutoPlay = false;
}

void ALocalTTSDemoController::StoreError(const FString& ErrorMessage)
{
	LastErrorMessage = ErrorMessage;
	AppendEventLog(FString::Printf(TEXT("错误：%s"), *ErrorMessage));
	UE_LOG(LogLocalTTSDemoController, Error, TEXT("%s"), *ErrorMessage);
	BroadcastStateUpdated();
}

void ALocalTTSDemoController::ClearError()
{
	LastErrorMessage.Reset();
}

void ALocalTTSDemoController::AppendEventLog(const FString& Message)
{
	if (Message.IsEmpty())
	{
		return;
	}

	EventLogLines.Add(Message);
	constexpr int32 MaxEventLogLines = 20;
	while (EventLogLines.Num() > MaxEventLogLines)
	{
		EventLogLines.RemoveAt(0);
	}

	EventLogText = FString::Join(EventLogLines, TEXT("\n"));
}

void ALocalTTSDemoController::BroadcastStateUpdated()
{
	OnDemoStateUpdated.Broadcast();
}

void ALocalTTSDemoController::UpdateSingleState(const ELocalTTSSpeakAsyncState NewState, const FString& DetailMessage)
{
	SingleState = NewState;
	SingleStateText = ULocalTTSBlueprintLibrary::GetLocalTTSSpeakAsyncStateText(NewState);
	if (!DetailMessage.IsEmpty())
	{
		SingleStateText = FString::Printf(TEXT("%s：%s"), *SingleStateText, *DetailMessage);
	}
	BroadcastStateUpdated();
}

void ALocalTTSDemoController::UpdateQueueState(const ELocalTTSLongTextQueueState NewState, const FString& DetailMessage)
{
	QueueState = NewState;
	QueueStateText = ULocalTTSBlueprintLibrary::GetLocalTTSLongTextQueueStateText(NewState);
	if (!DetailMessage.IsEmpty())
	{
		QueueStateText = FString::Printf(TEXT("%s：%s"), *QueueStateText, *DetailMessage);
	}
	BroadcastStateUpdated();
}
