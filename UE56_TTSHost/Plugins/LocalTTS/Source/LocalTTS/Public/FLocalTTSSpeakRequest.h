// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FLocalTTSSpeakRequest.generated.h"

USTRUCT(BlueprintType)
struct LOCALTTS_API FLocalTTSSpeakRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LocalTTS|Request", meta = (DisplayName = "合成文本", ToolTip = "必填。要让 OmniVoice 朗读的文本内容，建议先用一到两句中文短句测试，例如：你好，这是一次本地语音测试。"))
	FString Text;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LocalTTS|Request", meta = (DisplayName = "生成模式", ToolTip = "默认值为 auto。支持 auto、design、clone。auto 用默认声音生成；design 用 Instruct 标签设计音色；clone 使用参考音频进行音色克隆。"))
	FString Mode = TEXT("auto");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LocalTTS|Request", meta = (DisplayName = "语言 ID", ToolTip = "语言提示。常用值为 zh 或 en。蓝图预设节点默认使用 zh；留空时不向服务端发送 language_id。"))
	FString LanguageId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LocalTTS|Request|Reference", meta = (DisplayName = "参考音频路径", ToolTip = "clone 模式必填。填写本机存在的 wav 文件绝对路径，例如 D:\\voice\\ref.wav。当前 UE 侧会校验文件是否存在且扩展名为 wav。"))
	FString ReferenceAudioPath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LocalTTS|Request|Reference", meta = (DisplayName = "参考音频文本", ToolTip = "参考音频里实际说的话。clone 模式建议填写；当服务端 ASR 不可用时必须填写，否则无法自动识别参考音频内容。"))
	FString ReferenceText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LocalTTS|Request|Design", meta = (DisplayName = "音色标签", ToolTip = "design 模式必填。不是自由描述文本，只能使用 OmniVoice 支持的标签。英文示例：female, chinese accent。中文示例：女，青年，中音调。英文用半角逗号加空格，中文用全角逗号。"))
	FString Instruct;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LocalTTS|Request|Tuning", meta = (DisplayName = "目标时长", ClampMin = "0.0", ToolTip = "默认值为 0。单位为秒。0 表示不发送 duration，让模型自己决定语音时长；大于 0 时会作为目标时长发送给服务端，当前最大允许 60 秒。"))
	float Duration = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LocalTTS|Request|Tuning", meta = (DisplayName = "语速倍率", ClampMin = "0.1", ClampMax = "3.0", ToolTip = "默认值为 1.0，表示正常语速。小于 1 会更慢，大于 1 会更快；当前允许范围为大于 0 且小于等于 3。"))
	float Speed = 1.0f;
};
