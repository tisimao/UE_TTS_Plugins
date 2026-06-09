// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"

#include "FLocalTTSHealthResponse.h"
#include "ULocalTTSCheckHealthAsyncAction.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLocalTTSHealthSuccessDelegate, const FLocalTTSHealthResponse&, Response);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLocalTTSHealthFailureDelegate, const FString&, ErrorMessage);

UCLASS()
class LOCALTTS_API ULocalTTSCheckHealthAsyncAction : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FLocalTTSHealthSuccessDelegate OnSuccess;

	UPROPERTY(BlueprintAssignable)
	FLocalTTSHealthFailureDelegate OnFailure;

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"), Category = "LocalTTS")
	static ULocalTTSCheckHealthAsyncAction* CheckLocalTTSHealth(UObject* WorldContextObject);

	virtual void Activate() override;

private:
	void FinishWithFailure(const FString& ErrorMessage);

	TWeakObjectPtr<UObject> WorldContextObject;
};
