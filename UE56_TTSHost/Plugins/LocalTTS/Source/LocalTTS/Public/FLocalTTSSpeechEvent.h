// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "FLocalTTSEmotionFrame.h"
#include "FLocalTTSSpeakRequest.h"
#include "FLocalTTSTTSResponse.h"
#include "FLocalTTSVisemeFrame.h"
#include "FLocalTTSSpeechEvent.generated.h"

USTRUCT(BlueprintType)
struct LOCALTTS_API FLocalTTSSpeechEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS|SpeechEvent", meta = (DisplayName = "是否成功", ToolTip = "该语音事件是否来自一次成功的 TTS 响应。"))
	bool bOk = false;

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS|SpeechEvent", meta = (DisplayName = "请求 ID", ToolTip = "服务端请求编号，可用来对齐 UE 日志、服务端日志和生成的 wav 文件。"))
	FString RequestId;

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS|SpeechEvent", meta = (DisplayName = "合成文本", ToolTip = "生成本次语音时使用的文本。数字人系统可用它做字幕、情绪分析或后续口型辅助。"))
	FString Text;

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS|SpeechEvent", meta = (DisplayName = "生成模式", ToolTip = "本次语音使用的 OmniVoice 模式，例如 auto、design 或 clone。"))
	FString Mode;

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS|SpeechEvent", meta = (DisplayName = "语言 ID", ToolTip = "本次语音使用的语言提示，例如 zh 或 en。"))
	FString LanguageId;

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS|SpeechEvent", meta = (DisplayName = "WAV 路径", ToolTip = "生成的 wav 文件路径。后续数字人口型、表情或自定义播放系统优先从这里读取音频。"))
	FString WavPath;

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS|SpeechEvent", meta = (DisplayName = "采样率", ToolTip = "生成音频的采样率。OmniVoice 当前通常为 24000。"))
	int32 SampleRate = 0;

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS|SpeechEvent", meta = (DisplayName = "生成耗时毫秒", ToolTip = "服务端生成本次语音的耗时，单位为毫秒。注意这不是音频播放时长。"))
	int32 DurationMs = 0;

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS|SpeechEvent", meta = (DisplayName = "时长秒数", ToolTip = "语音时长秒数。当前优先根据服务端耗时或请求目标时长填充，后续可改为真实音频时长。"))
	float DurationSeconds = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS|SpeechEvent|DigitalHuman", meta = (DisplayName = "口型帧", ToolTip = "预留的口型帧数组。当前 OmniVoice 生成链路还不会填充，后续接 lip sync 分析器或 MetaHuman 驱动时使用。"))
	TArray<FLocalTTSVisemeFrame> VisemeFrames;

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS|SpeechEvent|DigitalHuman", meta = (DisplayName = "表情帧", ToolTip = "预留的表情帧数组。当前 OmniVoice 生成链路还不会填充，后续接情绪分析或数字人表情驱动时使用。"))
	TArray<FLocalTTSEmotionFrame> EmotionFrames;

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS|SpeechEvent|Error", meta = (DisplayName = "错误码", ToolTip = "语音事件对应的服务端错误码。"))
	FString ErrorCode;

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS|SpeechEvent|Error", meta = (DisplayName = "错误信息", ToolTip = "语音事件对应的服务端可读错误信息。"))
	FString ErrorMessage;

	static FLocalTTSSpeechEvent FromRequestAndResponse(
		const FLocalTTSSpeakRequest& SpeakRequest,
		const FLocalTTSTTSResponse& TTSResponse)
	{
		FLocalTTSSpeechEvent SpeechEvent;
		SpeechEvent.bOk = TTSResponse.bOk;
		SpeechEvent.RequestId = TTSResponse.RequestId;
		SpeechEvent.Text = SpeakRequest.Text;
		SpeechEvent.Mode = TTSResponse.Mode.IsEmpty() ? SpeakRequest.Mode : TTSResponse.Mode;
		SpeechEvent.LanguageId = SpeakRequest.LanguageId;
		SpeechEvent.WavPath = TTSResponse.WavPath;
		SpeechEvent.SampleRate = TTSResponse.SampleRate;
		SpeechEvent.DurationMs = TTSResponse.DurationMs;
		SpeechEvent.DurationSeconds = TTSResponse.DurationMs > 0 ? static_cast<float>(TTSResponse.DurationMs) / 1000.0f : SpeakRequest.Duration;
		SpeechEvent.ErrorCode = TTSResponse.ErrorCode;
		SpeechEvent.ErrorMessage = TTSResponse.ErrorMessage;
		return SpeechEvent;
	}
};
