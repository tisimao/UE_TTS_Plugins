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
	UPROPERTY(BlueprintAssignable, Category = "LocalTTS|Health")
	FLocalTTSHealthSuccessDelegate OnSuccess;

	UPROPERTY(BlueprintAssignable, Category = "LocalTTS|Health")
	FLocalTTSHealthFailureDelegate OnFailure;

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "检查 Local TTS 健康状态", ToolTip = "异步请求本地 OmniVoice 服务的 /health 接口，确认服务是否可访问、模型是否已经 ready。"), Category = "LocalTTS|Health")
	static ULocalTTSCheckHealthAsyncAction* CheckLocalTTSHealth(UObject* WorldContextObject);

	virtual void Activate() override;

private:
	void FinishWithFailure(const FString& ErrorMessage);

	TWeakObjectPtr<UObject> WorldContextObject;
};
