// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FLocalTTSHealthResponse.generated.h"

USTRUCT(BlueprintType)
struct LOCALTTS_API FLocalTTSHealthResponse
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS|Health", meta = (DisplayName = "是否成功", ToolTip = "本地 TTS 服务是否成功返回健康状态。true 通常表示服务可访问。"))
	bool bOk = false;

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS|Health", meta = (DisplayName = "服务名", ToolTip = "服务端返回的服务名称，用于确认当前访问的是 LocalTTS 服务。"))
	FString Service;

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS|Health", meta = (DisplayName = "服务状态", ToolTip = "服务端状态，例如 ready 或 loading。只有 ready 后才建议发送语音生成请求。"))
	FString Status;

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS|Health", meta = (DisplayName = "模型名", ToolTip = "服务端当前加载的模型名称。当前目标模型为 OmniVoice。"))
	FString Model;

	UPROPERTY(BlueprintReadOnly, Category = "LocalTTS|Health", meta = (DisplayName = "支持模式", ToolTip = "服务端支持的 TTS 模式列表，当前通常为 auto、clone、design。"))
	TArray<FString> SupportedModes;
};
