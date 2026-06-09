// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FLocalTTSVisemeFrame.generated.h"

USTRUCT(BlueprintType)
struct LOCALTTS_API FLocalTTSVisemeFrame
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LocalTTS|DigitalHuman|Viseme", meta = (DisplayName = "Time Seconds", ToolTip = "Time offset in seconds from the beginning of the generated speech."))
	float TimeSeconds = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LocalTTS|DigitalHuman|Viseme", meta = (DisplayName = "Viseme Name", ToolTip = "Mouth shape name, for example A, E, O, FV, MBP, or a MetaHuman-compatible viseme id."))
	FString VisemeName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LocalTTS|DigitalHuman|Viseme", meta = (DisplayName = "Weight", ClampMin = "0.0", ClampMax = "1.0", ToolTip = "Blend weight for this viseme frame."))
	float Weight = 1.0f;
};
