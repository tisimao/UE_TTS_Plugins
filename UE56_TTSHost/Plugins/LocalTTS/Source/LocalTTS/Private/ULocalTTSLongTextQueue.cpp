// Copyright Epic Games, Inc. All Rights Reserved.

#include "ULocalTTSLongTextQueue.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "ULocalTTSBlueprintLibrary.h"
#include "ULocalTTSSpeakAsyncAction.h"
#include "ULocalTTSSubsystem.h"

namespace LocalTTSLongTextQueue
{
	struct FCandidateSegment
	{
		FString Text;
		int32 SourceStart = 0;
		int32 SourceLength = 0;
	};

	static bool IsSplitDelimiter(const TCHAR Character, const bool bSplitOnNewLine)
	{
		return Character == TEXT('。')
			|| Character == TEXT('！')
			|| Character == TEXT('？')
			|| Character == TEXT('!')
			|| Character == TEXT('?')
			|| Character == TEXT(';')
			|| Character == TEXT('；')
			|| (bSplitOnNewLine && Character == TEXT('\n'));
	}

	static FString TrimCandidateText(const FString& InText)
	{
		return InText.TrimStartAndEnd();
	}

	static float EstimateRecommendedDuration(const FString& Text)
	{
		return Text.IsEmpty() ? 0.0f : FMath::Max(1.0f, static_cast<float>(Text.Len()) / 12.0f);
	}
}

bool ULocalTTSLongTextQueue::StartSpeakQueue(UObject* WorldContextObject, const FLocalTTSLongTextRequest& LongTextRequest, FString& ErrorMessage)
{
	return StartQueueInternal(WorldContextObject, LongTextRequest, true, ErrorMessage);
}

bool ULocalTTSLongTextQueue::StartGenerateQueue(UObject* WorldContextObject, const FLocalTTSLongTextRequest& LongTextRequest, FString& ErrorMessage)
{
	return StartQueueInternal(WorldContextObject, LongTextRequest, false, ErrorMessage);
}

void ULocalTTSLongTextQueue::StopQueue()
{
	bStopRequested = true;
	bPauseRequested = false;
	bSkipRequested = false;

	if (UObject* ContextObject = QueueWorldContextObject.Get())
	{
		ULocalTTSBlueprintLibrary::StopSpeaking(ContextObject);
	}

	ClearActiveAction();
	ClearPrefetchAction();
	PrefetchedSegmentIndex = INDEX_NONE;
	PrefetchedSegmentSpeechEvent = FLocalTTSSegmentSpeechEvent();
	PrefetchedSegmentTTSResponse = FLocalTTSTTSResponse();
	bPlayPrefetchedSegmentWhenReady = false;
	SetQueueState(ELocalTTSLongTextQueueState::Stopped, TEXT("长文本队列已手动停止。"));
	OnQueueStopped.Broadcast();
}

bool ULocalTTSLongTextQueue::PauseQueue(FString& ErrorMessage)
{
	if (QueueState == ELocalTTSLongTextQueueState::Paused)
	{
		return true;
	}

	if (QueueState == ELocalTTSLongTextQueueState::Idle
		|| QueueState == ELocalTTSLongTextQueueState::Finished
		|| QueueState == ELocalTTSLongTextQueueState::Stopped
		|| QueueState == ELocalTTSLongTextQueueState::Error)
	{
		ErrorMessage = TEXT("长文本队列当前不在运行中，无法暂停。");
		return false;
	}

	bPauseRequested = true;

	if (QueueState == ELocalTTSLongTextQueueState::Playing && bAutoPlaySegments)
	{
		UObject* ContextObject = QueueWorldContextObject.Get();
		if (!ContextObject || !ULocalTTSBlueprintLibrary::PauseSpeaking(ContextObject, ErrorMessage))
		{
			return false;
		}
	}

	SetQueueState(ELocalTTSLongTextQueueState::Paused, TEXT("长文本队列已暂停。"));
	return true;
}

bool ULocalTTSLongTextQueue::ResumeQueue(FString& ErrorMessage)
{
	if (QueueState != ELocalTTSLongTextQueueState::Paused)
	{
		ErrorMessage = TEXT("长文本队列当前不是暂停状态，无法继续。");
		return false;
	}

	bPauseRequested = false;

	if (bAutoPlaySegments && CurrentSegmentTTSResponse.bOk && !CurrentSegmentTTSResponse.WavPath.IsEmpty())
	{
		UObject* ContextObject = QueueWorldContextObject.Get();
		if (ContextObject && ULocalTTSBlueprintLibrary::ResumeSpeaking(ContextObject, ErrorMessage))
		{
			SetQueueState(ELocalTTSLongTextQueueState::Playing, TEXT("长文本队列已继续播放。"));
			return true;
		}

		if (!ErrorMessage.Contains(TEXT("没有已暂停")))
		{
			return false;
		}

		PlayCurrentGeneratedSegment();
		return true;
	}

	if (ActiveAction)
	{
		SetQueueState(ELocalTTSLongTextQueueState::Generating, TEXT("长文本队列已继续，正在等待当前段处理完成。"));
		return true;
	}

	SetQueueState(ELocalTTSLongTextQueueState::Generating, TEXT("长文本队列已继续，准备处理下一段。"));
	AdvanceQueue();
	return true;
}

bool ULocalTTSLongTextQueue::SkipToNext(FString& ErrorMessage)
{
	if (QueueState == ELocalTTSLongTextQueueState::Idle
		|| QueueState == ELocalTTSLongTextQueueState::Finished
		|| QueueState == ELocalTTSLongTextQueueState::Stopped
		|| QueueState == ELocalTTSLongTextQueueState::Error)
	{
		ErrorMessage = TEXT("长文本队列当前没有可跳过的段落。");
		return false;
	}

	if (!Segments.IsValidIndex(CurrentSegmentIndex))
	{
		ErrorMessage = TEXT("长文本队列当前段序号无效，无法跳过。");
		return false;
	}

	bPauseRequested = false;
	bSkipRequested = true;

	if (!ActiveAction)
	{
		if (bAutoPlaySegments)
		{
			if (UObject* ContextObject = QueueWorldContextObject.Get())
			{
				ULocalTTSBlueprintLibrary::StopSpeaking(ContextObject);
			}
		}

		bSkipRequested = false;
		CurrentSegmentSpeechEvent = FLocalTTSSegmentSpeechEvent();
		CurrentSegmentTTSResponse = FLocalTTSTTSResponse();
		SetQueueState(ELocalTTSLongTextQueueState::Generating, TEXT("已跳过当前段，准备处理下一段。"));
		AdvanceQueue();
		return true;
	}

	if (QueueState == ELocalTTSLongTextQueueState::Playing && bAutoPlaySegments)
	{
		const int32 NextSegmentIndex = CurrentSegmentIndex + 1;
		if (!Segments.IsValidIndex(NextSegmentIndex))
		{
			ErrorMessage = TEXT("当前已经是最后一段，没有可提前准备的下一段。");
			return false;
		}

		bSkipRequested = false;
		return StartPrefetchForSegment(NextSegmentIndex, ErrorMessage);
	}

	SetQueueState(ELocalTTSLongTextQueueState::Generating, TEXT("已请求跳过当前段，将在当前请求返回后进入下一段。"));
	return true;
}

ELocalTTSLongTextQueueState ULocalTTSLongTextQueue::GetQueueState() const
{
	return QueueState;
}

int32 ULocalTTSLongTextQueue::GetCurrentSegmentIndex() const
{
	return CurrentSegmentIndex;
}

TArray<FLocalTTSTextSegment> ULocalTTSLongTextQueue::GetSegments() const
{
	return Segments;
}

TArray<FLocalTTSSegmentSpeechEvent> ULocalTTSLongTextQueue::GetCompletedSegmentEvents() const
{
	return CompletedSegmentEvents;
}

void ULocalTTSLongTextQueue::HandleActiveActionStateChanged(const ELocalTTSSpeakAsyncState State, const FString& DetailMessage)
{
	if (bStopRequested)
	{
		return;
	}

	if (bPauseRequested && State != ELocalTTSSpeakAsyncState::Error)
	{
		return;
	}

	switch (State)
	{
	case ELocalTTSSpeakAsyncState::Started:
	case ELocalTTSSpeakAsyncState::WaitingForService:
	case ELocalTTSSpeakAsyncState::Generating:
	case ELocalTTSSpeakAsyncState::AudioReady:
		SetQueueState(ELocalTTSLongTextQueueState::Generating, DetailMessage);
		break;
	case ELocalTTSSpeakAsyncState::Playing:
		SetQueueState(ELocalTTSLongTextQueueState::Generating, DetailMessage);
		break;
	case ELocalTTSSpeakAsyncState::Finished:
		// Queue completion is finalized by OnFinished or OnSucceeded, so we avoid
		// broadcasting an intermediate queue state here.
		break;
	case ELocalTTSSpeakAsyncState::Error:
		SetQueueState(ELocalTTSLongTextQueueState::Error, DetailMessage);
		break;
	}
}

void ULocalTTSLongTextQueue::HandleActiveActionSpeechEventReady(const FLocalTTSSpeechEvent& SpeechEvent)
{
	if (bStopRequested || bSkipRequested || !Segments.IsValidIndex(CurrentSegmentIndex))
	{
		return;
	}

	CurrentSegmentSpeechEvent.Segment = Segments[CurrentSegmentIndex];
	CurrentSegmentSpeechEvent.SpeechEvent = SpeechEvent;
	OnSegmentGenerated.Broadcast(CurrentSegmentSpeechEvent);
}

void ULocalTTSLongTextQueue::HandleActiveActionError(const FString& ErrorMessage)
{
	if (bStopRequested)
	{
		ClearActiveAction();
		return;
	}

	FailCurrentSegment(ErrorMessage);
}

void ULocalTTSLongTextQueue::HandleActiveActionSucceeded(const FLocalTTSTTSResponse& Response)
{
	if (bStopRequested)
	{
		ClearActiveAction();
		return;
	}

	CurrentSegmentTTSResponse = Response;
	if (!Response.bOk)
	{
		const FString ErrorMessage = Response.ErrorMessage.IsEmpty()
			? TEXT("长文本队列当前段生成失败：服务端返回失败响应。")
			: Response.ErrorMessage;
		ClearActiveAction();
		FailCurrentSegment(ErrorMessage);
		return;
	}

	if (bSkipRequested)
	{
		bSkipRequested = false;
		CurrentSegmentSpeechEvent = FLocalTTSSegmentSpeechEvent();
		CurrentSegmentTTSResponse = FLocalTTSTTSResponse();
		ClearActiveAction();
		AdvanceQueue();
		return;
	}

	ClearActiveAction();

	if (bPauseRequested)
	{
		if (!bAutoPlaySegments)
		{
			CompleteCurrentSegment();
			bPauseRequested = true;
		}

		SetQueueState(ELocalTTSLongTextQueueState::Paused, TEXT("长文本队列已暂停在段落生成完成后。"));
		return;
	}

	if (bAutoPlaySegments)
	{
		PlayCurrentGeneratedSegment();
		return;
	}

	CompleteCurrentSegment();
}

void ULocalTTSLongTextQueue::HandlePrefetchActionSpeechEventReady(const FLocalTTSSpeechEvent& SpeechEvent)
{
	if (bStopRequested || !Segments.IsValidIndex(PrefetchedSegmentIndex))
	{
		return;
	}

	PrefetchedSegmentSpeechEvent.Segment = Segments[PrefetchedSegmentIndex];
	PrefetchedSegmentSpeechEvent.SpeechEvent = SpeechEvent;
}

void ULocalTTSLongTextQueue::HandlePrefetchActionError(const FString& ErrorMessage)
{
	ClearPrefetchAction();
	PrefetchedSegmentIndex = INDEX_NONE;
	PrefetchedSegmentSpeechEvent = FLocalTTSSegmentSpeechEvent();
	PrefetchedSegmentTTSResponse = FLocalTTSTTSResponse();

	if (!bStopRequested)
	{
		SetQueueState(QueueState, FString::Printf(TEXT("提前准备下一段失败：%s"), *ErrorMessage));
		if (bPlayPrefetchedSegmentWhenReady)
		{
			bPlayPrefetchedSegmentWhenReady = false;
			AdvanceQueue();
		}
	}
}

void ULocalTTSLongTextQueue::HandlePrefetchActionSucceeded(const FLocalTTSTTSResponse& Response)
{
	if (bStopRequested)
	{
		ClearPrefetchAction();
		return;
	}

	if (!Response.bOk)
	{
		const FString ErrorMessage = Response.ErrorMessage.IsEmpty()
			? TEXT("提前准备下一段失败：服务端返回失败响应。")
			: Response.ErrorMessage;
		HandlePrefetchActionError(ErrorMessage);
		return;
	}

	PrefetchedSegmentTTSResponse = Response;
	if (PrefetchedSegmentSpeechEvent.Segment.Text.IsEmpty() && Segments.IsValidIndex(PrefetchedSegmentIndex))
	{
		PrefetchedSegmentSpeechEvent.Segment = Segments[PrefetchedSegmentIndex];
	}

	ClearPrefetchAction();
	SetQueueState(QueueState, FString::Printf(TEXT("下一段已提前准备完成：第 %d/%d 段。"), PrefetchedSegmentIndex + 1, Segments.Num()));

	if (bPlayPrefetchedSegmentWhenReady && PrefetchedSegmentIndex == CurrentSegmentIndex + 1)
	{
		bPlayPrefetchedSegmentWhenReady = false;
		const int32 NextSegmentIndex = PrefetchedSegmentIndex;
		CurrentSegmentIndex = NextSegmentIndex;
		CurrentSegmentSpeechEvent = PrefetchedSegmentSpeechEvent;
		CurrentSegmentTTSResponse = PrefetchedSegmentTTSResponse;
		PrefetchedSegmentIndex = INDEX_NONE;
		PrefetchedSegmentSpeechEvent = FLocalTTSSegmentSpeechEvent();
		PrefetchedSegmentTTSResponse = FLocalTTSTTSResponse();
		OnSegmentStarted.Broadcast(Segments[CurrentSegmentIndex]);
		OnSegmentGenerated.Broadcast(CurrentSegmentSpeechEvent);
		PlayCurrentGeneratedSegment();
	}
}

bool ULocalTTSLongTextQueue::StartQueueInternal(
	UObject* InWorldContextObject,
	const FLocalTTSLongTextRequest& LongTextRequest,
	const bool bInAutoPlay,
	FString& ErrorMessage)
{
	if (!InWorldContextObject)
	{
		ErrorMessage = TEXT("长文本队列启动失败：World 上下文无效。");
		return false;
	}

	if (ActiveAction)
	{
		ErrorMessage = TEXT("长文本队列仍有未完成的段落任务，请先停止或等待当前队列结束。");
		return false;
	}

	if (QueueState != ELocalTTSLongTextQueueState::Idle
		&& QueueState != ELocalTTSLongTextQueueState::Finished
		&& QueueState != ELocalTTSLongTextQueueState::Stopped
		&& QueueState != ELocalTTSLongTextQueueState::Error)
	{
		ErrorMessage = TEXT("长文本队列当前仍在运行，不能重复启动。");
		return false;
	}

	if (LongTextRequest.SourceText.TrimStartAndEnd().IsEmpty())
	{
		ErrorMessage = TEXT("长文本队列启动失败：原始长文本为空。");
		return false;
	}

	QueueWorldContextObject = InWorldContextObject;
	ActiveRequest = LongTextRequest;
	Segments = BuildSegments(LongTextRequest);
	if (Segments.Num() == 0)
	{
		ErrorMessage = TEXT("长文本队列启动失败：拆分后没有得到可用文本段。");
		return false;
	}

	CompletedSegmentEvents.Reset();
	CurrentSegmentSpeechEvent = FLocalTTSSegmentSpeechEvent();
	CurrentSegmentTTSResponse = FLocalTTSTTSResponse();
	PrefetchedSegmentSpeechEvent = FLocalTTSSegmentSpeechEvent();
	PrefetchedSegmentTTSResponse = FLocalTTSTTSResponse();
	CurrentSegmentIndex = INDEX_NONE;
	PrefetchedSegmentIndex = INDEX_NONE;
	bAutoPlaySegments = bInAutoPlay;
	bStopRequested = false;
	bPauseRequested = false;
	bSkipRequested = false;
	bPlayPrefetchedSegmentWhenReady = false;

	SetQueueState(
		ELocalTTSLongTextQueueState::Segmenting,
		FString::Printf(TEXT("长文本已拆分完成，共 %d 段。"), Segments.Num()));
	OnQueueStarted.Broadcast();
	AdvanceQueue();
	return true;
}

void ULocalTTSLongTextQueue::AdvanceQueue()
{
	if (bStopRequested)
	{
		return;
	}

	if (bPauseRequested)
	{
		SetQueueState(ELocalTTSLongTextQueueState::Paused, TEXT("长文本队列已暂停，等待继续。"));
		return;
	}

	if (ActiveAction)
	{
		return;
	}

	const int32 NextSegmentIndex = CurrentSegmentIndex + 1;
	if (!Segments.IsValidIndex(NextSegmentIndex))
	{
		SetQueueState(ELocalTTSLongTextQueueState::Finished, TEXT("长文本队列已全部处理完成。"));
		OnQueueFinished.Broadcast();
		return;
	}

	CurrentSegmentIndex = NextSegmentIndex;
	StartCurrentSegment();
}

void ULocalTTSLongTextQueue::StartCurrentSegment()
{
	if (!Segments.IsValidIndex(CurrentSegmentIndex))
	{
		FailCurrentSegment(TEXT("长文本队列内部错误：当前段序号无效。"));
		return;
	}

	FLocalTTSTextSegment CurrentSegment = Segments[CurrentSegmentIndex];
	CurrentSegmentSpeechEvent = FLocalTTSSegmentSpeechEvent();
	OnSegmentStarted.Broadcast(CurrentSegment);
	SetQueueState(
		ELocalTTSLongTextQueueState::Generating,
		FString::Printf(TEXT("正在处理第 %d/%d 段。"), CurrentSegmentIndex + 1, Segments.Num()));

	FLocalTTSSpeakRequest SegmentRequest = ActiveRequest.SpeakRequestTemplate;
	SegmentRequest.Text = CurrentSegment.Text;
	if (ActiveRequest.bUseRecommendedDuration && SegmentRequest.Duration <= 0.0f)
	{
		SegmentRequest.Duration = CurrentSegment.RecommendedDuration;
	}

	ActiveAction = ULocalTTSSpeakAsyncAction::GenerateSpeechAsync(QueueWorldContextObject.Get(), SegmentRequest);

	if (!ActiveAction)
	{
		FailCurrentSegment(TEXT("长文本队列启动失败：无法创建段落异步语音动作。"));
		return;
	}

	ActiveAction->OnStateChanged.AddDynamic(this, &ULocalTTSLongTextQueue::HandleActiveActionStateChanged);
	ActiveAction->OnSpeechEventReady.AddDynamic(this, &ULocalTTSLongTextQueue::HandleActiveActionSpeechEventReady);
	ActiveAction->OnError.AddDynamic(this, &ULocalTTSLongTextQueue::HandleActiveActionError);
	ActiveAction->OnSucceeded.AddDynamic(this, &ULocalTTSLongTextQueue::HandleActiveActionSucceeded);
	ActiveAction->Activate();
}

void ULocalTTSLongTextQueue::CompleteCurrentSegment(const bool bAdvanceQueue)
{
	if (Segments.IsValidIndex(CurrentSegmentIndex))
	{
		if (CurrentSegmentSpeechEvent.Segment.Text.IsEmpty())
		{
			CurrentSegmentSpeechEvent.Segment = Segments[CurrentSegmentIndex];
		}

		CompletedSegmentEvents.Add(CurrentSegmentSpeechEvent);
		OnSegmentFinished.Broadcast(CurrentSegmentSpeechEvent);
	}

	CurrentSegmentSpeechEvent = FLocalTTSSegmentSpeechEvent();
	CurrentSegmentTTSResponse = FLocalTTSTTSResponse();
	ClearActiveAction();

	if (bPauseRequested)
	{
		SetQueueState(ELocalTTSLongTextQueueState::Paused, TEXT("长文本队列已暂停在段落边界。"));
		return;
	}

	if (bAdvanceQueue)
	{
		AdvanceQueue();
	}
}

void ULocalTTSLongTextQueue::FailCurrentSegment(const FString& ErrorMessage)
{
	const int32 FailedSegmentIndex = CurrentSegmentIndex;
	const FString FailedSegmentText = Segments.IsValidIndex(FailedSegmentIndex) ? Segments[FailedSegmentIndex].Text : FString();
	ClearActiveAction();
	SetQueueState(ELocalTTSLongTextQueueState::Error, ErrorMessage);
	OnQueueError.Broadcast(FailedSegmentIndex, FailedSegmentText, ErrorMessage);
}

void ULocalTTSLongTextQueue::ClearActiveAction()
{
	if (!ActiveAction)
	{
		return;
	}

	ActiveAction->OnStateChanged.RemoveAll(this);
	ActiveAction->OnSpeechEventReady.RemoveAll(this);
	ActiveAction->OnError.RemoveAll(this);
	ActiveAction->OnSucceeded.RemoveAll(this);
	ActiveAction = nullptr;
}

void ULocalTTSLongTextQueue::ClearPrefetchAction()
{
	if (PrefetchAction)
	{
		PrefetchAction->OnSpeechEventReady.RemoveAll(this);
		PrefetchAction->OnError.RemoveAll(this);
		PrefetchAction->OnSucceeded.RemoveAll(this);
		PrefetchAction = nullptr;
	}
}

void ULocalTTSLongTextQueue::SetQueueState(const ELocalTTSLongTextQueueState NewState, const FString& DetailMessage)
{
	QueueState = NewState;
	OnQueueStateChanged.Broadcast(NewState, DetailMessage);
}

bool ULocalTTSLongTextQueue::StartPrefetchForSegment(const int32 SegmentIndex, FString& ErrorMessage)
{
	if (!Segments.IsValidIndex(SegmentIndex))
	{
		ErrorMessage = TEXT("下一段序号无效，无法提前准备。");
		return false;
	}

	if (HasPrefetchedSegment(SegmentIndex))
	{
		SetQueueState(QueueState, FString::Printf(TEXT("下一段已提前准备完成：第 %d/%d 段。"), SegmentIndex + 1, Segments.Num()));
		return true;
	}

	if (PrefetchAction)
	{
		ErrorMessage = TEXT("下一段正在提前准备中，请稍后。");
		return false;
	}

	FLocalTTSSpeakRequest SegmentRequest = ActiveRequest.SpeakRequestTemplate;
	SegmentRequest.Text = Segments[SegmentIndex].Text;
	if (ActiveRequest.bUseRecommendedDuration && SegmentRequest.Duration <= 0.0f)
	{
		SegmentRequest.Duration = Segments[SegmentIndex].RecommendedDuration;
	}

	PrefetchAction = ULocalTTSSpeakAsyncAction::GenerateSpeechAsync(QueueWorldContextObject.Get(), SegmentRequest);
	if (!PrefetchAction)
	{
		ErrorMessage = TEXT("提前准备下一段失败：无法创建段落异步语音动作。");
		return false;
	}

	PrefetchedSegmentIndex = SegmentIndex;
	PrefetchedSegmentSpeechEvent = FLocalTTSSegmentSpeechEvent();
	PrefetchedSegmentTTSResponse = FLocalTTSTTSResponse();
	PrefetchAction->OnSpeechEventReady.AddDynamic(this, &ULocalTTSLongTextQueue::HandlePrefetchActionSpeechEventReady);
	PrefetchAction->OnError.AddDynamic(this, &ULocalTTSLongTextQueue::HandlePrefetchActionError);
	PrefetchAction->OnSucceeded.AddDynamic(this, &ULocalTTSLongTextQueue::HandlePrefetchActionSucceeded);
	PrefetchAction->Activate();

	SetQueueState(QueueState, FString::Printf(TEXT("当前段继续播放，正在提前准备第 %d/%d 段。"), SegmentIndex + 1, Segments.Num()));
	return true;
}

bool ULocalTTSLongTextQueue::HasPrefetchedSegment(const int32 SegmentIndex) const
{
	return PrefetchedSegmentIndex == SegmentIndex
		&& PrefetchedSegmentTTSResponse.bOk
		&& !PrefetchedSegmentTTSResponse.WavPath.IsEmpty();
}

void ULocalTTSLongTextQueue::PlayCurrentGeneratedSegment()
{
	ULocalTTSSubsystem* Subsystem = ResolveSubsystem();
	if (!Subsystem)
	{
		FailCurrentSegment(TEXT("长文本队列播放失败：LocalTTS 子系统不可用。"));
		return;
	}

	if (!CurrentSegmentTTSResponse.bOk || CurrentSegmentTTSResponse.WavPath.IsEmpty())
	{
		FailCurrentSegment(TEXT("长文本队列播放失败：当前段没有有效的 WAV 结果。"));
		return;
	}

	UObject* ContextObject = QueueWorldContextObject.Get();
	if (!ContextObject)
	{
		FailCurrentSegment(TEXT("长文本队列播放失败：World 上下文无效。"));
		return;
	}

	SetQueueState(
		ELocalTTSLongTextQueueState::Playing,
		FString::Printf(TEXT("正在播放第 %d/%d 段。"), CurrentSegmentIndex + 1, Segments.Num()));

	Subsystem->PlaySpeech(
		ContextObject,
		CurrentSegmentTTSResponse,
		[]()
		{
		},
		[this]()
		{
			if (bStopRequested)
			{
				return;
			}

			if (bSkipRequested)
			{
				bSkipRequested = false;
				CurrentSegmentSpeechEvent = FLocalTTSSegmentSpeechEvent();
				CurrentSegmentTTSResponse = FLocalTTSTTSResponse();
				AdvanceQueue();
				return;
			}

			const int32 NextSegmentIndex = CurrentSegmentIndex + 1;
			CompleteCurrentSegment(false);

			if (HasPrefetchedSegment(NextSegmentIndex))
			{
				CurrentSegmentIndex = NextSegmentIndex;
				CurrentSegmentSpeechEvent = PrefetchedSegmentSpeechEvent;
				CurrentSegmentTTSResponse = PrefetchedSegmentTTSResponse;
				PrefetchedSegmentIndex = INDEX_NONE;
				PrefetchedSegmentSpeechEvent = FLocalTTSSegmentSpeechEvent();
				PrefetchedSegmentTTSResponse = FLocalTTSTTSResponse();
				OnSegmentStarted.Broadcast(Segments[CurrentSegmentIndex]);
				OnSegmentGenerated.Broadcast(CurrentSegmentSpeechEvent);
				PlayCurrentGeneratedSegment();
				return;
			}

			if (PrefetchAction && PrefetchedSegmentIndex == NextSegmentIndex)
			{
				bPlayPrefetchedSegmentWhenReady = true;
				SetQueueState(ELocalTTSLongTextQueueState::Generating, FString::Printf(TEXT("当前段播放完成，等待第 %d/%d 段提前准备完成。"), NextSegmentIndex + 1, Segments.Num()));
				return;
			}

			AdvanceQueue();
		},
		[this](const FString& ErrorMessage)
		{
			if (!bStopRequested)
			{
				FailCurrentSegment(ErrorMessage);
			}
		});
}

ULocalTTSSubsystem* ULocalTTSLongTextQueue::ResolveSubsystem() const
{
	UObject* ContextObject = QueueWorldContextObject.Get();
	if (!ContextObject || !GEngine)
	{
		return nullptr;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(ContextObject, EGetWorldErrorMode::ReturnNull);
	if (!World)
	{
		return nullptr;
	}

	UGameInstance* GameInstance = World->GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<ULocalTTSSubsystem>() : nullptr;
}

TArray<FLocalTTSTextSegment> ULocalTTSLongTextQueue::BuildSegments(const FLocalTTSLongTextRequest& LongTextRequest) const
{
	using namespace LocalTTSLongTextQueue;

	TArray<FCandidateSegment> RawCandidates;
	const FString& SourceText = LongTextRequest.SourceText;
	const int32 MaxCharacters = FMath::Max(8, LongTextRequest.MaxCharactersPerSegment);
	const int32 MinCharacters = FMath::Max(0, LongTextRequest.MinCharactersPerSegment);

	FString CurrentBuffer;
	int32 CurrentStart = INDEX_NONE;

	for (int32 CharacterIndex = 0; CharacterIndex < SourceText.Len(); ++CharacterIndex)
	{
		const TCHAR Character = SourceText[CharacterIndex];
		if (Character == TEXT('\r'))
		{
			continue;
		}

		if (CurrentStart == INDEX_NONE)
		{
			CurrentStart = CharacterIndex;
		}

		CurrentBuffer.AppendChar(Character);
		if (!IsSplitDelimiter(Character, LongTextRequest.bSplitOnNewLine))
		{
			continue;
		}

		const FString TrimmedText = TrimCandidateText(CurrentBuffer);
		if (!TrimmedText.IsEmpty())
		{
			FCandidateSegment Candidate;
			Candidate.Text = TrimmedText;
			Candidate.SourceStart = CurrentStart;
			Candidate.SourceLength = CharacterIndex - CurrentStart + 1;
			RawCandidates.Add(Candidate);
		}

		CurrentBuffer.Reset();
		CurrentStart = INDEX_NONE;
	}

	const FString RemainingText = TrimCandidateText(CurrentBuffer);
	if (!RemainingText.IsEmpty())
	{
		FCandidateSegment Candidate;
		Candidate.Text = RemainingText;
		Candidate.SourceStart = CurrentStart == INDEX_NONE ? 0 : CurrentStart;
		Candidate.SourceLength = CurrentBuffer.Len();
		RawCandidates.Add(Candidate);
	}

	TArray<FCandidateSegment> SplitCandidates;
	for (const FCandidateSegment& Candidate : RawCandidates)
	{
		if (Candidate.Text.Len() <= MaxCharacters)
		{
			SplitCandidates.Add(Candidate);
			continue;
		}

		for (int32 Offset = 0; Offset < Candidate.Text.Len(); Offset += MaxCharacters)
		{
			FCandidateSegment SplitCandidate;
			SplitCandidate.Text = Candidate.Text.Mid(Offset, MaxCharacters);
			SplitCandidate.SourceStart = Candidate.SourceStart + Offset;
			SplitCandidate.SourceLength = SplitCandidate.Text.Len();
			SplitCandidates.Add(SplitCandidate);
		}
	}

	TArray<FCandidateSegment> PackedCandidates;
	for (const FCandidateSegment& Candidate : SplitCandidates)
	{
		if (!PackedCandidates.IsEmpty() && PackedCandidates.Last().Text.Len() + Candidate.Text.Len() <= MaxCharacters)
		{
			if (!PackedCandidates.Last().Text.EndsWith(TEXT("\n")) && !Candidate.Text.StartsWith(TEXT("\n")))
			{
				PackedCandidates.Last().Text += TEXT(" ");
			}

			PackedCandidates.Last().Text += Candidate.Text;
			PackedCandidates.Last().SourceLength = (Candidate.SourceStart + Candidate.SourceLength) - PackedCandidates.Last().SourceStart;
			continue;
		}

		PackedCandidates.Add(Candidate);
	}

	if (MinCharacters > 0)
	{
		for (int32 SegmentIndex = 1; SegmentIndex < PackedCandidates.Num(); ++SegmentIndex)
		{
			if (PackedCandidates[SegmentIndex].Text.Len() >= MinCharacters)
			{
				continue;
			}

			const int32 SeparatorLength = PackedCandidates[SegmentIndex - 1].Text.EndsWith(TEXT("\n")) ? 0 : 1;
			if (PackedCandidates[SegmentIndex - 1].Text.Len() + SeparatorLength + PackedCandidates[SegmentIndex].Text.Len() > MaxCharacters)
			{
				continue;
			}

			if (SeparatorLength > 0)
			{
				PackedCandidates[SegmentIndex - 1].Text += TEXT(" ");
			}

			PackedCandidates[SegmentIndex - 1].Text += PackedCandidates[SegmentIndex].Text;
			PackedCandidates[SegmentIndex - 1].SourceLength =
				(PackedCandidates[SegmentIndex].SourceStart + PackedCandidates[SegmentIndex].SourceLength) - PackedCandidates[SegmentIndex - 1].SourceStart;
			PackedCandidates.RemoveAt(SegmentIndex);
			--SegmentIndex;
		}
	}

	TArray<FLocalTTSTextSegment> BuiltSegments;
	for (int32 SegmentIndex = 0; SegmentIndex < PackedCandidates.Num(); ++SegmentIndex)
	{
		FLocalTTSTextSegment Segment;
		Segment.SegmentIndex = SegmentIndex;
		Segment.Text = PackedCandidates[SegmentIndex].Text;
		Segment.SourceStart = PackedCandidates[SegmentIndex].SourceStart;
		Segment.SourceLength = PackedCandidates[SegmentIndex].SourceLength;
		Segment.RecommendedDuration = EstimateRecommendedDuration(Segment.Text);
		BuiltSegments.Add(Segment);
	}

	return BuiltSegments;
}
