// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class USoundWaveProcedural;

class LOCALTTS_API FLocalTTSWavLoader
{
public:
	bool ValidateWavFile(const FString& WavPath, FString& OutErrorMessage) const;

	USoundWaveProcedural* LoadSoundWave(
		UObject* Outer,
		const FString& WavPath,
		FString& OutErrorMessage) const;
};
