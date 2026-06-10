// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "ELocalTTSSpeakAsyncState.h"
#include "ELocalTTSLongTextQueueState.h"
#include "FLocalTTSLongTextRequest.h"
#include "FLocalTTSSegmentSpeechEvent.h"
#include "FLocalTTSTTSResponse.h"
#include "FLocalTTSTextSegment.h"
#include "FLocalTTSSpeechEvent.h"
#include "ULocalTTSLongTextQueue.generated.h"

class ULocalTTSSpeakAsyncAction;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLocalTTSLongTextQueueSimpleDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLocalTTSLongTextQueueStateChangedDelegate, ELocalTTSLongTextQueueState, State, const FString&, DetailMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLocalTTSLongTextSegmentDelegate, const FLocalTTSTextSegment&, Segment);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLocalTTSSegmentSpeechEventDelegate, const FLocalTTSSegmentSpeechEvent&, SegmentSpeechEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FLocalTTSLongTextQueueErrorDelegate, int32, SegmentIndex, const FString&, SegmentText, const FString&, ErrorMessage);

// 长文本队列控制器，负责分段、逐段生成、顺序播放，以及向 UI/字幕/数字人广播队列进度。
UCLASS(BlueprintType)
class LOCALTTS_API ULocalTTSLongTextQueue : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "LocalTTS|LongText", meta = (DisplayName = "队列开始", ToolTip = "长文本队列正式开始处理时触发。"))
	FLocalTTSLongTextQueueSimpleDelegate OnQueueStarted;

	UPROPERTY(BlueprintAssignable, Category = "LocalTTS|LongText", meta = (DisplayName = "队列状态变化", ToolTip = "长文本队列的统一状态回调，可直接驱动 UI 的“分段中 / 生成中 / 播放中 / 已完成 / 失败”等状态。"))
	FLocalTTSLongTextQueueStateChangedDelegate OnQueueStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "LocalTTS|LongText", meta = (DisplayName = "段落开始", ToolTip = "某个文本段开始进入处理流程时触发。"))
	FLocalTTSLongTextSegmentDelegate OnSegmentStarted;

	UPROPERTY(BlueprintAssignable, Category = "LocalTTS|LongText", meta = (DisplayName = "段落已生成", ToolTip = "某个文本段已经生成出语音事件时触发。"))
	FLocalTTSSegmentSpeechEventDelegate OnSegmentGenerated;

	UPROPERTY(BlueprintAssignable, Category = "LocalTTS|LongText", meta = (DisplayName = "段落完成", ToolTip = "某个文本段完全处理结束时触发。自动播放模式下表示该段播放结束，仅生成模式下表示该段生成结束。"))
	FLocalTTSSegmentSpeechEventDelegate OnSegmentFinished;

	UPROPERTY(BlueprintAssignable, Category = "LocalTTS|LongText", meta = (DisplayName = "队列完成", ToolTip = "所有文本段都已顺序处理完成时触发。"))
	FLocalTTSLongTextQueueSimpleDelegate OnQueueFinished;

	UPROPERTY(BlueprintAssignable, Category = "LocalTTS|LongText", meta = (DisplayName = "队列停止", ToolTip = "长文本队列被手动停止时触发。"))
	FLocalTTSLongTextQueueSimpleDelegate OnQueueStopped;

	UPROPERTY(BlueprintAssignable, Category = "LocalTTS|LongText", meta = (DisplayName = "队列错误", ToolTip = "某个文本段处理失败时触发，同时会给出段序号、段文本和错误原因。"))
	FLocalTTSLongTextQueueErrorDelegate OnQueueError;

	UFUNCTION(BlueprintCallable, Category = "LocalTTS|LongText", meta = (WorldContext = "WorldContextObject", DisplayName = "开始长文本生成并播放", ToolTip = "把长文本拆成多个文本段后，逐段生成并顺序播放。"))
	bool StartSpeakQueue(UObject* WorldContextObject, const FLocalTTSLongTextRequest& LongTextRequest, FString& ErrorMessage);

	UFUNCTION(BlueprintCallable, Category = "LocalTTS|LongText", meta = (WorldContext = "WorldContextObject", DisplayName = "开始长文本仅生成", ToolTip = "把长文本拆成多个文本段后，逐段生成语音事件，但不自动播放。"))
	bool StartGenerateQueue(UObject* WorldContextObject, const FLocalTTSLongTextRequest& LongTextRequest, FString& ErrorMessage);

	UFUNCTION(BlueprintCallable, Category = "LocalTTS|LongText", meta = (DisplayName = "停止长文本队列", ToolTip = "停止当前长文本队列，并尝试中断当前播放。"))
	void StopQueue();

	UFUNCTION(BlueprintCallable, Category = "LocalTTS|LongText", meta = (DisplayName = "暂停长文本队列", ToolTip = "暂停当前长文本队列。播放中会暂停当前音频；生成中会在当前段处理完成后停在段落边界。"))
	bool PauseQueue(FString& ErrorMessage);

	UFUNCTION(BlueprintCallable, Category = "LocalTTS|LongText", meta = (DisplayName = "继续长文本队列", ToolTip = "继续已暂停的长文本队列。"))
	bool ResumeQueue(FString& ErrorMessage);

	UFUNCTION(BlueprintCallable, Category = "LocalTTS|LongText", meta = (DisplayName = "跳到下一段", ToolTip = "跳过当前文本段并继续处理下一段。生成中跳过会等待当前请求回调返回后再进入下一段。"))
	bool SkipToNext(FString& ErrorMessage);

	UFUNCTION(BlueprintPure, Category = "LocalTTS|LongText", meta = (DisplayName = "获取长文本队列状态", ToolTip = "读取当前长文本队列状态。"))
	ELocalTTSLongTextQueueState GetQueueState() const;

	UFUNCTION(BlueprintPure, Category = "LocalTTS|LongText", meta = (DisplayName = "获取当前段序号", ToolTip = "读取当前正在处理的文本段序号。没有有效段落时返回 -1。"))
	int32 GetCurrentSegmentIndex() const;

	UFUNCTION(BlueprintPure, Category = "LocalTTS|LongText", meta = (DisplayName = "获取已拆分文本段", ToolTip = "读取本次长文本任务拆分出来的所有文本段。"))
	TArray<FLocalTTSTextSegment> GetSegments() const;

	UFUNCTION(BlueprintPure, Category = "LocalTTS|LongText", meta = (DisplayName = "获取已完成段事件", ToolTip = "读取当前已完成处理的文本段语音事件。适合字幕或数字人系统补拉历史结果。"))
	TArray<FLocalTTSSegmentSpeechEvent> GetCompletedSegmentEvents() const;

private:
	UFUNCTION()
	void HandleActiveActionStateChanged(ELocalTTSSpeakAsyncState State, const FString& DetailMessage);

	UFUNCTION()
	void HandleActiveActionSpeechEventReady(const FLocalTTSSpeechEvent& SpeechEvent);

	UFUNCTION()
	void HandleActiveActionError(const FString& ErrorMessage);

	UFUNCTION()
	void HandleActiveActionSucceeded(const FLocalTTSTTSResponse& Response);

	bool StartQueueInternal(UObject* WorldContextObject, const FLocalTTSLongTextRequest& LongTextRequest, bool bInAutoPlay, FString& ErrorMessage);
	void AdvanceQueue();
	void StartCurrentSegment();
	void CompleteCurrentSegment();
	void FailCurrentSegment(const FString& ErrorMessage);
	void ClearActiveAction();
	void SetQueueState(ELocalTTSLongTextQueueState NewState, const FString& DetailMessage);
	TArray<FLocalTTSTextSegment> BuildSegments(const FLocalTTSLongTextRequest& LongTextRequest) const;
	void PlayCurrentGeneratedSegment();
	class ULocalTTSSubsystem* ResolveSubsystem() const;

	TWeakObjectPtr<UObject> QueueWorldContextObject;
	FLocalTTSLongTextRequest ActiveRequest;
	TArray<FLocalTTSTextSegment> Segments;
	TArray<FLocalTTSSegmentSpeechEvent> CompletedSegmentEvents;
	FLocalTTSSegmentSpeechEvent CurrentSegmentSpeechEvent;
	FLocalTTSTTSResponse CurrentSegmentTTSResponse;

	UPROPERTY(Transient)
	TObjectPtr<ULocalTTSSpeakAsyncAction> ActiveAction = nullptr;

	ELocalTTSLongTextQueueState QueueState = ELocalTTSLongTextQueueState::Idle;
	int32 CurrentSegmentIndex = INDEX_NONE;
	bool bAutoPlaySegments = true;
	bool bStopRequested = false;
	bool bPauseRequested = false;
	bool bSkipRequested = false;
};
