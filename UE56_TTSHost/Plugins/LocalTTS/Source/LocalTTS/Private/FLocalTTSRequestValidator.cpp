// Copyright Epic Games, Inc. All Rights Reserved.

#include "FLocalTTSRequestValidator.h"

#include "Misc/Paths.h"

bool FLocalTTSRequestValidator::Validate(const FLocalTTSSpeakRequest& SpeakRequest, FString& OutErrorMessage) const
{
	if (SpeakRequest.Text.TrimStartAndEnd().IsEmpty())
	{
		OutErrorMessage = TEXT("合成文本不能为空。请在 Text / 合成文本 中填写要朗读的内容。");
		return false;
	}

	if (!ValidateMode(SpeakRequest, OutErrorMessage))
	{
		return false;
	}

	if (!SpeakRequest.LanguageId.TrimStartAndEnd().IsEmpty())
	{
		const FString NormalizedLanguageId = SpeakRequest.LanguageId.TrimStartAndEnd().ToLower();
		if (NormalizedLanguageId != TEXT("zh") && NormalizedLanguageId != TEXT("en"))
		{
			OutErrorMessage = FString::Printf(
				TEXT("不支持的 LanguageId：%s。当前支持 zh、en 或留空。"),
				*SpeakRequest.LanguageId);
			return false;
		}
	}

	if (SpeakRequest.Speed <= 0.0f || SpeakRequest.Speed > 3.0f)
	{
		OutErrorMessage = TEXT("语速 Speed 必须大于 0 且小于等于 3。默认建议使用 1.0。");
		return false;
	}

	if (SpeakRequest.Duration < 0.0f)
	{
		OutErrorMessage = TEXT("目标时长 Duration 不能小于 0。0 表示让模型自己决定时长。");
		return false;
	}

	if (SpeakRequest.Duration > 60.0f)
	{
		OutErrorMessage = TEXT("目标时长 Duration 当前不能超过 60 秒。");
		return false;
	}

	return true;
}

FString FLocalTTSRequestValidator::NormalizeMode(const FString& Mode) const
{
	const FString TrimmedMode = Mode.TrimStartAndEnd();
	return TrimmedMode.IsEmpty() ? TEXT("auto") : TrimmedMode.ToLower();
}

bool FLocalTTSRequestValidator::ValidateMode(const FLocalTTSSpeakRequest& SpeakRequest, FString& OutErrorMessage) const
{
	const FString NormalizedMode = NormalizeMode(SpeakRequest.Mode);
	if (NormalizedMode == TEXT("auto"))
	{
		return true;
	}

	if (NormalizedMode == TEXT("clone"))
	{
		return ValidateCloneMode(SpeakRequest, OutErrorMessage);
	}

	if (NormalizedMode == TEXT("design"))
	{
		return ValidateDesignMode(SpeakRequest, OutErrorMessage);
	}

	OutErrorMessage = FString::Printf(TEXT("不支持的生成模式：%s。当前支持 auto、design、clone。"), *SpeakRequest.Mode);
	return false;
}

bool FLocalTTSRequestValidator::ValidateCloneMode(
	const FLocalTTSSpeakRequest& SpeakRequest,
	FString& OutErrorMessage) const
{
	if (SpeakRequest.ReferenceAudioPath.TrimStartAndEnd().IsEmpty())
	{
		OutErrorMessage = TEXT("clone 模式需要填写 ReferenceAudioPath / 参考音频路径。");
		return false;
	}

	const FString ReferenceAudioPath = SpeakRequest.ReferenceAudioPath.TrimStartAndEnd();
	if (!FPaths::FileExists(ReferenceAudioPath))
	{
		OutErrorMessage = FString::Printf(TEXT("参考音频文件不存在：%s"), *ReferenceAudioPath);
		return false;
	}

	const FString Extension = FPaths::GetExtension(ReferenceAudioPath).ToLower();
	if (Extension != TEXT("wav"))
	{
		OutErrorMessage = TEXT("clone 模式当前要求参考音频为 wav 文件。");
		return false;
	}

	if (!SpeakRequest.ReferenceText.TrimStartAndEnd().IsEmpty() && SpeakRequest.ReferenceText.TrimStartAndEnd().Len() < 2)
	{
		OutErrorMessage = TEXT("ReferenceText / 参考音频文本过短。请填写参考音频里实际说的话，或留空让服务端 ASR 尝试识别。");
		return false;
	}

	return true;
}

bool FLocalTTSRequestValidator::ValidateDesignMode(
	const FLocalTTSSpeakRequest& SpeakRequest,
	FString& OutErrorMessage) const
{
	const FString Instruct = SpeakRequest.Instruct.TrimStartAndEnd();
	if (Instruct.IsEmpty())
	{
		OutErrorMessage = TEXT("design 模式需要填写 Instruct / 音色标签。示例：female, chinese accent 或 女，青年，中音调。");
		return false;
	}

	return ValidateDesignInstruct(Instruct, OutErrorMessage);
}

bool FLocalTTSRequestValidator::ValidateDesignInstruct(
	const FString& Instruct,
	FString& OutErrorMessage) const
{
	const bool bUseChineseItems = ContainsNonAscii(Instruct);
	const FString ExpectedDelimiter = bUseChineseItems ? TEXT("，") : TEXT(", ");
	if (!bUseChineseItems && Instruct.Contains(TEXT("，")))
	{
		OutErrorMessage = TEXT("英文 Instruct 标签请使用半角逗号加空格分隔，例如：female, chinese accent。");
		return false;
	}

	if (bUseChineseItems && Instruct.Contains(TEXT(",")))
	{
		OutErrorMessage = TEXT("中文 Instruct 标签请使用全角逗号分隔，例如：女，青年，中音调。");
		return false;
	}

	TArray<FString> Items;
	Instruct.ParseIntoArray(Items, *ExpectedDelimiter, true);
	if (Items.Num() == 0)
	{
		OutErrorMessage = TEXT("design 模式至少需要一个 OmniVoice 支持的 Instruct 标签。");
		return false;
	}

	for (FString Item : Items)
	{
		Item = Item.TrimStartAndEnd();
		const bool bSupported = bUseChineseItems
			? IsSupportedChineseInstructItem(Item)
			: IsSupportedEnglishInstructItem(Item.ToLower());
		if (!bSupported)
		{
			OutErrorMessage = FString::Printf(
				TEXT("不支持的 Instruct 标签：%s。请只使用 OmniVoice 支持的标签，例如：female, chinese accent 或 女，青年，中音调。"),
				*Item);
			return false;
		}
	}

	return true;
}

bool FLocalTTSRequestValidator::IsSupportedEnglishInstructItem(const FString& Item) const
{
	static const TSet<FString> SupportedItems = {
		TEXT("american accent"),
		TEXT("australian accent"),
		TEXT("british accent"),
		TEXT("canadian accent"),
		TEXT("child"),
		TEXT("chinese accent"),
		TEXT("elderly"),
		TEXT("female"),
		TEXT("high pitch"),
		TEXT("indian accent"),
		TEXT("japanese accent"),
		TEXT("korean accent"),
		TEXT("low pitch"),
		TEXT("male"),
		TEXT("middle-aged"),
		TEXT("moderate pitch"),
		TEXT("portuguese accent"),
		TEXT("russian accent"),
		TEXT("teenager"),
		TEXT("very high pitch"),
		TEXT("very low pitch"),
		TEXT("whisper"),
		TEXT("young adult")
	};
	return SupportedItems.Contains(Item);
}

bool FLocalTTSRequestValidator::IsSupportedChineseInstructItem(const FString& Item) const
{
	static const TSet<FString> SupportedItems = {
		TEXT("东北话"),
		TEXT("中年"),
		TEXT("中音调"),
		TEXT("云南话"),
		TEXT("低音调"),
		TEXT("儿童"),
		TEXT("四川话"),
		TEXT("女"),
		TEXT("宁夏话"),
		TEXT("少年"),
		TEXT("极低音调"),
		TEXT("极高音调"),
		TEXT("桂林话"),
		TEXT("河南话"),
		TEXT("济南话"),
		TEXT("甘肃话"),
		TEXT("男"),
		TEXT("石家庄话"),
		TEXT("老年"),
		TEXT("耳语"),
		TEXT("贵州话"),
		TEXT("陕西话"),
		TEXT("青岛话"),
		TEXT("青年"),
		TEXT("高音调")
	};
	return SupportedItems.Contains(Item);
}

bool FLocalTTSRequestValidator::ContainsNonAscii(const FString& Value) const
{
	for (const TCHAR Character : Value)
	{
		if (Character > 127)
		{
			return true;
		}
	}
	return false;
}
