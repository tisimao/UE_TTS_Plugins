// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "ELocalTTSErrorCode.h"
#include "ELocalTTSServiceState.h"
#include "FLocalTTSHealthResponse.h"
#include "FLocalTTSSpeakRequest.h"
#include "FLocalTTSTTSResponse.h"
#include "ULocalTTSBlueprintLibrary.generated.h"

UCLASS()
class LOCALTTS_API ULocalTTSBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "LocalTTS|Request", meta = (DisplayName = "创建 Auto 语音请求", ToolTip = "创建 OmniVoice auto 模式请求。适合最常用的文字转语音测试；LanguageId 为空时默认 zh，Speed 小于等于 0 时默认 1.0。"))
	static FLocalTTSSpeakRequest MakeAutoSpeakRequest(const FString& Text, const FString& LanguageId, float Speed);

	UFUNCTION(BlueprintPure, Category = "LocalTTS|Request", meta = (DisplayName = "创建 Design 语音请求", ToolTip = "创建 OmniVoice design 模式请求。Instruct 只能填写模型支持的音色标签；为空时默认 female, chinese accent。中文示例：女，青年，中音调。"))
	static FLocalTTSSpeakRequest MakeDesignSpeakRequest(const FString& Text, const FString& Instruct, const FString& LanguageId, float Speed);

	UFUNCTION(BlueprintPure, Category = "LocalTTS|Request", meta = (DisplayName = "创建 Clone 语音请求", ToolTip = "创建 OmniVoice clone 模式请求。ReferenceAudioPath 必须是本机存在的 wav 文件；ReferenceText 建议填写，服务端 ASR 不可用时必须填写。"))
	static FLocalTTSSpeakRequest MakeCloneSpeakRequest(const FString& Text, const FString& ReferenceAudioPath, const FString& ReferenceText, const FString& LanguageId, float Speed);

	UFUNCTION(BlueprintCallable, Category = "LocalTTS|Request", meta = (DisplayName = "校验 Local TTS 请求", ToolTip = "发送到本地 OmniVoice 服务前先校验请求参数。会检查文本、模式、语言、语速、时长、clone 参考音频和 design 音色标签。"))
	static bool ValidateSpeakRequest(const FLocalTTSSpeakRequest& SpeakRequest, FString& ErrorMessage);

	UFUNCTION(BlueprintCallable, Category = "LocalTTS|Service", meta = (WorldContext = "WorldContextObject", DisplayName = "启动 Local TTS", ToolTip = "启动或预热本地 OmniVoice TTS 服务。服务启动后仍需要等待模型加载完成，Health 状态 ready 后再请求语音。"))
	static bool StartLocalTTS(const UObject* WorldContextObject, FString& ErrorMessage);

	UFUNCTION(BlueprintCallable, Category = "LocalTTS|Service", meta = (WorldContext = "WorldContextObject", DisplayName = "停止 Local TTS", ToolTip = "停止由插件启动的本地 TTS 服务进程，同时停止当前播放。"))
	static void StopLocalTTS(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "LocalTTS|Playback", meta = (WorldContext = "WorldContextObject", DisplayName = "停止 Local TTS 播放", ToolTip = "停止当前正在播放的 LocalTTS 语音，并释放运行时 AudioComponent。"))
	static void StopSpeaking(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "LocalTTS|Service", meta = (WorldContext = "WorldContextObject", DisplayName = "Local TTS 是否 Ready", ToolTip = "当本地 TTS 服务 /health 返回 ready 时为 true。首次加载模型时可能需要等待。"))
	static bool IsLocalTTSReady(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "LocalTTS|Service", meta = (WorldContext = "WorldContextObject", DisplayName = "获取 Local TTS 服务状态", ToolTip = "获取当前服务状态枚举，例如 Stopped、Starting、Ready、Busy、Error。"))
	static ELocalTTSServiceState GetLocalTTSServiceState(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "LocalTTS|Service", meta = (WorldContext = "WorldContextObject", DisplayName = "获取 Local TTS 服务状态文本", ToolTip = "获取当前服务状态的可读文本，方便蓝图调试显示。"))
	static FString GetLocalTTSServiceStateText(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "LocalTTS|Playback", meta = (WorldContext = "WorldContextObject", DisplayName = "Local TTS 是否正在播放", ToolTip = "当前 LocalTTS 音频正在 UE 内播放时返回 true。"))
	static bool IsSpeaking(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "LocalTTS|Service", meta = (WorldContext = "WorldContextObject", DisplayName = "Local TTS 是否忙碌", ToolTip = "生成语音请求进行中或音频正在播放时返回 true。可用于避免重复点击 Speak。"))
	static bool IsLocalTTSBusy(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "LocalTTS|Debug", meta = (WorldContext = "WorldContextObject", DisplayName = "获取最近一次健康响应", ToolTip = "读取插件缓存的最近一次 /health 响应，包括 ok、status、model 和 supported_modes。用于蓝图调试和 UI 显示。"))
	static FLocalTTSHealthResponse GetLastLocalTTSHealth(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "LocalTTS|Debug", meta = (WorldContext = "WorldContextObject", DisplayName = "获取最近一次健康错误", ToolTip = "读取最近一次健康检查失败时的错误文本。服务未启动、端口错误或模型未 ready 时可先看这里。"))
	static FString GetLastLocalTTSError(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "LocalTTS|Debug", meta = (WorldContext = "WorldContextObject", DisplayName = "获取最近一次健康错误码", ToolTip = "读取最近一次健康检查失败时的错误码，方便蓝图按错误类型分支处理。"))
	static ELocalTTSErrorCode GetLastLocalTTSHealthErrorCode(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "LocalTTS|Debug", meta = (WorldContext = "WorldContextObject", DisplayName = "获取最近一次语音结果", ToolTip = "读取最近一次 /tts 成功响应，包括 request_id、wav 路径、采样率和推理耗时。"))
	static FLocalTTSTTSResponse GetLastLocalTTSSpeechResult(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "LocalTTS|Debug", meta = (WorldContext = "WorldContextObject", DisplayName = "获取最近一次 WAV 路径", ToolTip = "读取最近一次成功生成的 wav 文件路径。可用于确认文件是否生成，或交给数字人/自定义播放器使用。"))
	static FString GetLastLocalTTSWavPath(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "LocalTTS|Debug", meta = (WorldContext = "WorldContextObject", DisplayName = "获取最近一次语音错误", ToolTip = "读取最近一次语音生成、wav 加载或播放失败时的错误文本。参数错误和 OmniVoice 推理错误通常会出现在这里。"))
	static FString GetLastLocalTTSSpeechError(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "LocalTTS|Debug", meta = (WorldContext = "WorldContextObject", DisplayName = "获取最近一次语音错误码", ToolTip = "读取最近一次语音生成、wav 加载或播放失败时的错误码，方便蓝图按错误类型分支处理。"))
	static ELocalTTSErrorCode GetLastLocalTTSSpeechErrorCode(const UObject* WorldContextObject);

private:
	static class ULocalTTSSubsystem* ResolveSubsystem(const UObject* WorldContextObject);
};
