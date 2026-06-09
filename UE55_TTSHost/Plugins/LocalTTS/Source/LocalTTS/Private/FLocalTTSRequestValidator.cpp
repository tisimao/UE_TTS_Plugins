// Copyright Epic Games, Inc. All Rights Reserved.

#include "FLocalTTSRequestValidator.h"

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

	if (SpeakRequest.Speed < 0.0f)
	{
		OutErrorMessage = TEXT("Speed must not be negative.");
		return false;
	}

	if (SpeakRequest.Duration < 0.0f)
	{
		OutErrorMessage = TEXT("Duration must not be negative.");
		return false;
	}

	return true;
}

bool FLocalTTSRequestValidator::ValidateMode(const FLocalTTSSpeakRequest& SpeakRequest, FString& OutErrorMessage) const
{
	const FString NormalizedMode = SpeakRequest.Mode.IsEmpty() ? TEXT("auto") : SpeakRequest.Mode.ToLower();
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
