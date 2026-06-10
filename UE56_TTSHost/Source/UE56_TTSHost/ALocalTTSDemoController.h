// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "ELocalTTSLongTextQueueState.h"
#include "ELocalTTSSpeakAsyncState.h"
#include "FLocalTTSHealthResponse.h"
#include "FLocalTTSLongTextRequest.h"
#include "FLocalTTSSegmentSpeechEvent.h"
#include "FLocalTTSSpeakRequest.h"
#include "FLocalTTSSpeechEvent.h"
#include "FLocalTTSTTSResponse.h"
#include "ALocalTTSDemoController.generated.h"

class ULocalTTSLongTextQueue;
class ULocalTTSSubsystem;
class ULocalTTSSpeakAsyncAction;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLocalTTSDemoControllerUpdatedDelegate);

UCLASS(
	BlueprintType,
	Blueprintable,
	meta = (
		DisplayName = "LocalTTS Demo 控制器",
		ToolTip = "轻量 Demo UI 和关卡使用的控制器 Actor。它只负责把正式 LocalTTS 蓝图入口包装成 UI 友好的状态和按钮操作。"
	))
class UE56_TTSHOST_API ALocalTTSDemoController : public AActor
{
	GENERATED_BODY()

public:
	ALocalTTSDemoController();
	virtual void BeginPlay() override;

	UPROPERTY(BlueprintAssignable, Category = "LocalTTS Demo", meta = (DisplayName = "Demo 状态已更新", ToolTip = "当服务、单句、长文本或错误信息变化时触发。Widget 可以监听它刷新界面。"))
	FLocalTTSDemoControllerUpdatedDelegate OnDemoStateUpdated;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LocalTTS Demo|输入", meta = (DisplayName = "默认单句文本", MultiLine = "true", ToolTip = "UI 没有传入文本时，单句生成会使用这里的示例文本。"))
	FString DefaultSingleText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LocalTTS Demo|输入", meta = (DisplayName = "默认长文本", MultiLine = "true", ToolTip = "UI 没有传入文本时，长文本队列会使用这里的示例文本。"))
	FString DefaultLongText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LocalTTS Demo|请求", meta = (DisplayName = "单句请求模板", ToolTip = "单句生成使用的默认请求参数。按钮传入文本后，会覆盖其中的 Text 字段。"))
	FLocalTTSSpeakRequest SingleSpeakRequestTemplate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LocalTTS Demo|请求", meta = (DisplayName = "长文本请求模板", ToolTip = "长文本队列使用的默认请求参数和分段策略。按钮传入文本后，会覆盖其中的 SourceText 字段。"))
	FLocalTTSLongTextRequest LongTextRequestTemplate;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LocalTTS Demo|状态", meta = (DisplayName = "服务状态文本"))
	FString ServiceStateText;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LocalTTS Demo|状态", meta = (DisplayName = "单句状态"))
	ELocalTTSSpeakAsyncState SingleState = ELocalTTSSpeakAsyncState::Idle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LocalTTS Demo|状态", meta = (DisplayName = "单句状态文本"))
	FString SingleStateText;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LocalTTS Demo|状态", meta = (DisplayName = "长文本队列状态"))
	ELocalTTSLongTextQueueState QueueState = ELocalTTSLongTextQueueState::Idle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LocalTTS Demo|状态", meta = (DisplayName = "长文本队列状态文本"))
	FString QueueStateText;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LocalTTS Demo|长文本", meta = (DisplayName = "当前段序号"))
	int32 CurrentSegmentIndex = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LocalTTS Demo|长文本", meta = (DisplayName = "总段数"))
	int32 TotalSegmentCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LocalTTS Demo|长文本", meta = (DisplayName = "当前段文本", MultiLine = "true"))
	FString CurrentSegmentText;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LocalTTS Demo|结果", meta = (DisplayName = "最近健康响应"))
	FLocalTTSHealthResponse LastHealthResponse;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LocalTTS Demo|结果", meta = (DisplayName = "最近语音响应"))
	FLocalTTSTTSResponse LastTTSResponse;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LocalTTS Demo|结果", meta = (DisplayName = "最近语音事件"))
	FLocalTTSSpeechEvent LastSpeechEvent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LocalTTS Demo|结果", meta = (DisplayName = "最近长文本段事件"))
	FLocalTTSSegmentSpeechEvent LastSegmentSpeechEvent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LocalTTS Demo|结果", meta = (DisplayName = "最近 WAV 路径"))
	FString LastWavPath;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LocalTTS Demo|结果", meta = (DisplayName = "最近请求 ID"))
	FString LastRequestId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LocalTTS Demo|结果", meta = (DisplayName = "最近错误提示", MultiLine = "true"))
	FString LastErrorMessage;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LocalTTS Demo|结果", meta = (DisplayName = "事件日志", MultiLine = "true"))
	FString EventLogText;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LocalTTS Demo|长文本", meta = (DisplayName = "长文本队列实例", ToolTip = "必须保存引用，避免队列 UObject 被回收。"))
	TObjectPtr<ULocalTTSLongTextQueue> LongTextQueue = nullptr;

	UFUNCTION(BlueprintCallable, Category = "LocalTTS Demo|服务", meta = (DisplayName = "Demo 启动服务", ToolTip = "请求启动本地 LocalTTS 服务。注意：启动成功不等于模型已就绪，仍建议继续检查健康状态。"))
	bool StartService(FString& ErrorMessage);

	UFUNCTION(BlueprintCallable, Category = "LocalTTS Demo|服务", meta = (DisplayName = "Demo 检查健康", ToolTip = "异步检查 LocalTTS 服务健康状态，并把结果写入 Demo 状态字段。"))
	bool CheckHealth(FString& ErrorMessage);

	UFUNCTION(BlueprintCallable, Category = "LocalTTS Demo|单句", meta = (DisplayName = "Demo 单句生成并播放", ToolTip = "使用正式异步语音节点生成并播放单句语音。Text 为空时使用默认单句文本。"))
	bool SpeakSingle(const FString& Text, FString& ErrorMessage);

	UFUNCTION(BlueprintCallable, Category = "LocalTTS Demo|单句", meta = (DisplayName = "Demo 单句仅生成 WAV", ToolTip = "使用正式异步语音节点仅生成 WAV，不自动播放。Text 为空时使用默认单句文本。"))
	bool GenerateSingle(const FString& Text, FString& ErrorMessage);

	UFUNCTION(BlueprintCallable, Category = "LocalTTS Demo|单句", meta = (DisplayName = "Demo 停止单句播放", ToolTip = "停止当前正在播放的 LocalTTS 音频。生成中的服务端请求不会被强制取消。"))
	void StopSingle();

	UFUNCTION(BlueprintCallable, Category = "LocalTTS Demo|单句", meta = (DisplayName = "Demo 暂停单句播放", ToolTip = "暂停当前正在播放的 LocalTTS 音频。"))
	bool PauseSingle(FString& ErrorMessage);

	UFUNCTION(BlueprintCallable, Category = "LocalTTS Demo|单句", meta = (DisplayName = "Demo 继续单句播放", ToolTip = "继续当前已暂停的 LocalTTS 音频。"))
	bool ResumeSingle(FString& ErrorMessage);

	UFUNCTION(BlueprintCallable, Category = "LocalTTS Demo|长文本", meta = (DisplayName = "Demo 创建或重置长文本队列", ToolTip = "创建一个新的长文本队列并绑定状态回调。后续 UI 按钮都操作这个队列实例。"))
	ULocalTTSLongTextQueue* CreateOrResetLongTextQueue();

	UFUNCTION(BlueprintCallable, Category = "LocalTTS Demo|长文本", meta = (DisplayName = "Demo 长文本生成并播放", ToolTip = "把长文本拆分为多段，然后按顺序生成并播放。Text 为空时使用默认长文本。"))
	bool StartLongTextSpeak(const FString& Text, FString& ErrorMessage);

	UFUNCTION(BlueprintCallable, Category = "LocalTTS Demo|长文本", meta = (DisplayName = "Demo 长文本仅生成", ToolTip = "把长文本拆分为多段，然后按顺序仅生成 WAV，不自动播放。Text 为空时使用默认长文本。"))
	bool StartLongTextGenerate(const FString& Text, FString& ErrorMessage);

	UFUNCTION(BlueprintCallable, Category = "LocalTTS Demo|长文本", meta = (DisplayName = "Demo 暂停长文本", ToolTip = "暂停长文本队列。播放中会暂停当前音频；生成中会在当前段处理完成后停在段落边界。"))
	bool PauseLongText(FString& ErrorMessage);

	UFUNCTION(BlueprintCallable, Category = "LocalTTS Demo|长文本", meta = (DisplayName = "Demo 继续长文本", ToolTip = "继续已暂停的长文本队列。"))
	bool ResumeLongText(FString& ErrorMessage);

	UFUNCTION(BlueprintCallable, Category = "LocalTTS Demo|长文本", meta = (DisplayName = "Demo 跳到下一段", ToolTip = "跳过当前长文本段并继续处理下一段。生成中的段会等待回调返回后再进入下一段。"))
	bool SkipLongText(FString& ErrorMessage);

	UFUNCTION(BlueprintCallable, Category = "LocalTTS Demo|长文本", meta = (DisplayName = "Demo 停止长文本", ToolTip = "停止当前长文本队列，并尝试停止正在播放的音频。"))
	void StopLongText();

	UFUNCTION(BlueprintCallable, Category = "LocalTTS Demo|状态", meta = (DisplayName = "Demo 刷新状态文本", ToolTip = "手动刷新服务、单句和队列状态文本，适合 Widget 构造时调用。"))
	void RefreshStateTexts();

	UFUNCTION(BlueprintCallable, Category = "LocalTTS Demo|状态", meta = (DisplayName = "Demo 清空事件日志"))
	void ClearEventLog();

	UFUNCTION(BlueprintPure, Category = "LocalTTS Demo|状态", meta = (DisplayName = "Demo 获取长文本进度文本", ToolTip = "返回适合直接显示在 UI 上的长文本进度，例如 2 / 5。"))
	FString GetLongTextProgressText() const;

private:
	UFUNCTION()
	void HandleSingleStarted();

	UFUNCTION()
	void HandleSingleSucceeded(const FLocalTTSTTSResponse& Response);

	UFUNCTION()
	void HandleSingleAudioReady(const FLocalTTSTTSResponse& Response);

	UFUNCTION()
	void HandleSingleSpeechEventReady(const FLocalTTSSpeechEvent& SpeechEvent);

	UFUNCTION()
	void HandleSingleFinished();

	UFUNCTION()
	void HandleSingleError(const FString& ErrorMessage);

	UFUNCTION()
	void HandleSingleStateChanged(ELocalTTSSpeakAsyncState State, const FString& DetailMessage);

	UFUNCTION()
	void HandleQueueStarted();

	UFUNCTION()
	void HandleQueueStateChanged(ELocalTTSLongTextQueueState State, const FString& DetailMessage);

	UFUNCTION()
	void HandleSegmentStarted(const FLocalTTSTextSegment& Segment);

	UFUNCTION()
	void HandleSegmentGenerated(const FLocalTTSSegmentSpeechEvent& SegmentSpeechEvent);

	UFUNCTION()
	void HandleSegmentFinished(const FLocalTTSSegmentSpeechEvent& SegmentSpeechEvent);

	UFUNCTION()
	void HandleQueueFinished();

	UFUNCTION()
	void HandleQueueStopped();

	UFUNCTION()
	void HandleQueueError(int32 SegmentIndex, const FString& SegmentText, const FString& ErrorMessage);

	ULocalTTSSubsystem* ResolveSubsystem() const;
	FLocalTTSSpeakRequest BuildSingleRequest(const FString& Text) const;
	FLocalTTSLongTextRequest BuildLongTextRequest(const FString& Text) const;
	bool StartSingleInternal(const FString& Text, bool bAutoPlay, FString& ErrorMessage);
	bool StartLongTextInternal(const FString& Text, bool bAutoPlay, FString& ErrorMessage);
	void BindLongTextQueueCallbacks();
	void UnbindLongTextQueueCallbacks();
	void ClearActiveSingleAction();
	void StoreError(const FString& ErrorMessage);
	void ClearError();
	void AppendEventLog(const FString& Message);
	void BroadcastStateUpdated();
	void UpdateSingleState(ELocalTTSSpeakAsyncState NewState, const FString& DetailMessage);
	void UpdateQueueState(ELocalTTSLongTextQueueState NewState, const FString& DetailMessage);

	UPROPERTY(Transient)
	TObjectPtr<ULocalTTSSpeakAsyncAction> ActiveSingleAction = nullptr;

	TArray<FString> EventLogLines;
	bool bActiveSingleAutoPlay = false;
};
