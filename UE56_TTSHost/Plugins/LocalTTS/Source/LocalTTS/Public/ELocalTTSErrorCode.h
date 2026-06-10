// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ELocalTTSErrorCode.generated.h"

UENUM(BlueprintType)
enum class ELocalTTSErrorCode : uint8
{
	None UMETA(DisplayName = "无错误"),
	InternalError UMETA(DisplayName = "插件内部错误"),
	ServiceProcessError UMETA(DisplayName = "服务进程错误"),
	ServiceUnreachable UMETA(DisplayName = "服务不可访问"),
	HttpError UMETA(DisplayName = "HTTP 请求错误"),
	ParseError UMETA(DisplayName = "响应解析错误"),
	RequestValidationFailed UMETA(DisplayName = "请求参数校验失败"),
	ServiceReturnedError UMETA(DisplayName = "服务返回错误"),
	AlreadyBusy UMETA(DisplayName = "已有请求进行中"),
	WavFileInvalid UMETA(DisplayName = "WAV 文件无效"),
	WavLoadFailed UMETA(DisplayName = "WAV 加载失败"),
	PlaybackFailed UMETA(DisplayName = "播放失败")
};
