// Copyright Epic Games, Inc. All Rights Reserved.

#include "FLocalTTSRequestValidator.h"

#include "Misc/Paths.h"

bool FLocalTTSRequestValidator::Validate(const FLocalTTSSpeakRequest& SpeakRequest, FString& OutErrorMessage) const
{
	if (SpeakRequest.Text.TrimStartAndEnd().IsEmpty())
	{
		OutErrorMessage = TEXT("Text must not be empty.");
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
				TEXT("Unsupported LanguageId: %s. Supported values are zh, en, or empty."),
				*SpeakRequest.LanguageId);
			return false;
		}
	}

	if (SpeakRequest.Speed <= 0.0f || SpeakRequest.Speed > 3.0f)
	{
		OutErrorMessage = TEXT("Speed must be greater than 0 and less than or equal to 3.");
		return false;
	}

	if (SpeakRequest.Duration < 0.0f)
	{
		OutErrorMessage = TEXT("Duration must not be negative.");
		return false;
	}

	if (SpeakRequest.Duration > 60.0f)
	{
		OutErrorMessage = TEXT("Duration must be less than or equal to 60 seconds.");
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

	OutErrorMessage = FString::Printf(TEXT("Unsupported mode: %s"), *SpeakRequest.Mode);
	return false;
}

bool FLocalTTSRequestValidator::ValidateCloneMode(
	const FLocalTTSSpeakRequest& SpeakRequest,
	FString& OutErrorMessage) const
{
	if (SpeakRequest.ReferenceAudioPath.TrimStartAndEnd().IsEmpty())
	{
		OutErrorMessage = TEXT("Clone mode requires ReferenceAudioPath.");
		return false;
	}

	const FString ReferenceAudioPath = SpeakRequest.ReferenceAudioPath.TrimStartAndEnd();
	if (!FPaths::FileExists(ReferenceAudioPath))
	{
		OutErrorMessage = FString::Printf(TEXT("Reference audio does not exist: %s"), *ReferenceAudioPath);
		return false;
	}

	const FString Extension = FPaths::GetExtension(ReferenceAudioPath).ToLower();
	if (Extension != TEXT("wav"))
	{
		OutErrorMessage = TEXT("Clone mode currently expects a wav ReferenceAudioPath.");
		return false;
	}

	if (!SpeakRequest.ReferenceText.TrimStartAndEnd().IsEmpty() && SpeakRequest.ReferenceText.TrimStartAndEnd().Len() < 2)
	{
		OutErrorMessage = TEXT("ReferenceText is too short when provided.");
		return false;
	}

	return true;
}

bool FLocalTTSRequestValidator::ValidateDesignMode(
	const FLocalTTSSpeakRequest& SpeakRequest,
	FString& OutErrorMessage) const
{
	if (SpeakRequest.Instruct.TrimStartAndEnd().IsEmpty())
	{
		OutErrorMessage = TEXT("Design mode requires Instruct.");
		return false;
	}

	return true;
}
