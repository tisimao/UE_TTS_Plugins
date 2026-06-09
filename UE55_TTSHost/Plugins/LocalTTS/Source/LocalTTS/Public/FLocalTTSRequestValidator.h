// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "FLocalTTSSpeakRequest.h"

class LOCALTTS_API FLocalTTSRequestValidator
{
public:
	bool Validate(const FLocalTTSSpeakRequest& SpeakRequest, FString& OutErrorMessage) const;

private:
	bool ValidateMode(const FLocalTTSSpeakRequest& SpeakRequest, FString& OutErrorMessage) const;
	bool ValidateCloneMode(const FLocalTTSSpeakRequest& SpeakRequest, FString& OutErrorMessage) const;
	bool ValidateDesignMode(const FLocalTTSSpeakRequest& SpeakRequest, FString& OutErrorMessage) const;
};
