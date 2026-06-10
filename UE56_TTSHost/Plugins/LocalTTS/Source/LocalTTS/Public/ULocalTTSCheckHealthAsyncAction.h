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
	UPROPERTY(BlueprintAssignable, Category = "LocalTTS|Health", meta = (DisplayName = "检查成功", ToolTip = "成功拿到 /health 响应。可根据返回的 status 决定 UI 显示“就绪”还是“加载中”。"))
	FLocalTTSHealthSuccessDelegate OnSuccess;

	UPROPERTY(BlueprintAssignable, Category = "LocalTTS|Health", meta = (DisplayName = "检查失败", ToolTip = "健康检查失败。通常用于提示服务未启动、端口不通或模型未正确加载。"))
	FLocalTTSHealthFailureDelegate OnFailure;

	UFUNCTION(
		BlueprintCallable,
		meta = (
			BlueprintInternalUseOnly = "true",
			WorldContext = "WorldContextObject",
			DisplayName = "异步检查 Local TTS 健康",
			ToolTip = "异步请求本地 OmniVoice 服务的 /health 接口，用于确认服务是否可访问、模型是否已就绪。"
		),
		Category = "LocalTTS|Health")
	static ULocalTTSCheckHealthAsyncAction* CheckLocalTTSHealth(UObject* WorldContextObject);

	virtual void Activate() override;

private:
	void FinishWithFailure(const FString& ErrorMessage);

	TWeakObjectPtr<UObject> WorldContextObject;
};
