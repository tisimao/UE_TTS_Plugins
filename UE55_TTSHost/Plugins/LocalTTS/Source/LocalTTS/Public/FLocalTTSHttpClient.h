// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "FLocalTTSHealthResponse.h"
#include "FLocalTTSSpeakRequest.h"
#include "FLocalTTSTTSResponse.h"

class LOCALTTS_API FLocalTTSHttpClient
{
public:
	using FOnHealthSuccess = TFunction<void(const FLocalTTSHealthResponse&)>;
	using FOnTTSSuccess = TFunction<void(const FLocalTTSTTSResponse&)>;
	using FOnRequestFailure = TFunction<void(const FString&)>;

	void GetHealth(
		const FString& BaseUrl,
		FOnHealthSuccess&& OnSuccess,
		FOnRequestFailure&& OnFailure) const;

	void PostTTS(
		const FString& BaseUrl,
		const FLocalTTSSpeakRequest& SpeakRequest,
		FOnTTSSuccess&& OnSuccess,
		FOnRequestFailure&& OnFailure) const;
};
