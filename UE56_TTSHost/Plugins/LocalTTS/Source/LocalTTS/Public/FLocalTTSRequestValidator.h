// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "FLocalTTSSpeakRequest.h"

class LOCALTTS_API FLocalTTSRequestValidator
{
public:
	bool Validate(const FLocalTTSSpeakRequest& SpeakRequest, FString& OutErrorMessage) const;

private:
	FString NormalizeMode(const FString& Mode) const;
	bool ValidateMode(const FLocalTTSSpeakRequest& SpeakRequest, FString& OutErrorMessage) const;
	bool ValidateCloneMode(const FLocalTTSSpeakRequest& SpeakRequest, FString& OutErrorMessage) const;
	bool ValidateDesignMode(const FLocalTTSSpeakRequest& SpeakRequest, FString& OutErrorMessage) const;
	bool ValidateDesignInstruct(const FString& Instruct, FString& OutErrorMessage) const;
	bool IsSupportedEnglishInstructItem(const FString& Item) const;
	bool IsSupportedChineseInstructItem(const FString& Item) const;
	bool ContainsNonAscii(const FString& Value) const;
};
