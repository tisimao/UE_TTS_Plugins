// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"

#include "ULocalTTSSettings.generated.h"

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "LocalTTS"))
class LOCALTTS_API ULocalTTSSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Service", meta = (DisplayName = "服务地址", ToolTip = "默认值为 http://127.0.0.1:50021。UE 插件会通过这个地址访问本地 FastAPI TTS 服务的 /health 和 /tts 接口。"))
	FString ServiceBaseUrl = TEXT("http://127.0.0.1:50021");

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Service", meta = (DisplayName = "服务相对目录", ToolTip = "默认值为 Services/tts_service。相对于仓库根目录，用来定位 run_server.py、.venv、cache 和 logs。"))
	FString ServiceRelativeRoot = TEXT("Services/tts_service");

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Service", meta = (DisplayName = "Python 相对路径", ToolTip = "默认值为 .venv/Scripts/python.exe。相对于服务目录，用来启动本地 TTS 服务。首次部署请先运行 Setup_TTS_Service.bat 创建该虚拟环境。"))
	FString PythonRelativePath = TEXT(".venv/Scripts/python.exe");

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Service", meta = (DisplayName = "服务启动脚本", ToolTip = "默认值为 run_server.py。插件会在服务目录下用 Python 启动这个脚本。"))
	FString RunServerScriptName = TEXT("run_server.py");

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Health Poll", meta = (DisplayName = "最大健康检查次数", ClampMin = "1", ToolTip = "默认值为 30。点击 Speak 或 Gen 后，如果服务还没 ready，插件会按固定间隔轮询 /health，超过次数后报等待超时。"))
	int32 MaxHealthPollCount = 30;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Health Poll", meta = (DisplayName = "健康检查间隔秒数", ClampMin = "0.1", ToolTip = "默认值为 1 秒。配合最大健康检查次数，默认最多等待约 30 秒。首次加载 OmniVoice 较慢时可以适当调大最大健康检查次数。"))
	float HealthPollIntervalSeconds = 1.0f;
};
