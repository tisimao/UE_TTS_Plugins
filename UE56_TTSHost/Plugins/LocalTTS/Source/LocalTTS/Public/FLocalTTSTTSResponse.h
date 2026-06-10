// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FLocalTTSTTSResponse.generated.h"

USTRUCT(BlueprintType)
struct LOCALTTS_API FLocalTTSTTSResponse
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS|Response", meta = (DisplayName = "是否成功", ToolTip = "本次 TTS 请求是否成功。true 表示服务端已经生成 wav 文件。"))
	bool bOk = false;

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS|Response", meta = (DisplayName = "请求 ID", ToolTip = "服务端生成的请求编号，例如 req_000002。可用来对齐 UE 日志和服务端日志。"))
	FString RequestId;

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS|Response", meta = (DisplayName = "实际模式", ToolTip = "服务端实际使用的 OmniVoice 模式，例如 auto、design 或 clone。"))
	FString Mode;

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS|Response", meta = (DisplayName = "WAV 路径", ToolTip = "本次生成的 wav 文件本地路径。UE 播放和数字人分析都会使用这个文件。"))
	FString WavPath;

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS|Response", meta = (DisplayName = "采样率", ToolTip = "生成音频的采样率。OmniVoice 当前通常为 24000。"))
	int32 SampleRate = 0;

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS|Response", meta = (DisplayName = "生成耗时毫秒", ToolTip = "服务端完成本次语音生成花费的时间，单位为毫秒。注意这不是音频播放时长。"))
	int32 DurationMs = 0;

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS|Response|Error", meta = (DisplayName = "服务错误码", ToolTip = "服务端返回的错误码。请求失败时用于定位参数、推理或内部错误。"))
	FString ErrorCode;

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS|Response|Error", meta = (DisplayName = "服务错误信息", ToolTip = "服务端返回的可读错误信息。OmniVoice 推理失败、Instruct 标签不支持等问题会显示在这里。"))
	FString ErrorMessage;
};
