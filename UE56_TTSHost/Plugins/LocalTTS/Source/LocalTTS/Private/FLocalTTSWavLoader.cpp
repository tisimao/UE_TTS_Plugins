// Copyright Epic Games, Inc. All Rights Reserved.

#include "FLocalTTSWavLoader.h"

#include "Audio.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Sound/SoundWaveProcedural.h"

namespace LocalTTSWavLoader
{
	static bool LoadWavBytes(const FString& WavPath, TArray<uint8>& OutBytes, FString& OutErrorMessage)
	{
		if (!FPaths::FileExists(WavPath))
		{
			OutErrorMessage = FString::Printf(TEXT("WAV file does not exist: %s"), *WavPath);
			return false;
		}

		if (!FFileHelper::LoadFileToArray(OutBytes, *WavPath))
		{
			OutErrorMessage = FString::Printf(TEXT("Failed to read WAV file: %s"), *WavPath);
			return false;
		}

		return true;
	}
}

bool FLocalTTSWavLoader::ValidateWavFile(const FString& WavPath, FString& OutErrorMessage) const
{
	TArray<uint8> WavBytes;
	if (!LocalTTSWavLoader::LoadWavBytes(WavPath, WavBytes, OutErrorMessage))
	{
		return false;
	}

	FWaveModInfo WaveInfo;
	if (!WaveInfo.ReadWaveInfo(WavBytes.GetData(), WavBytes.Num()))
	{
		OutErrorMessage = FString::Printf(TEXT("Invalid WAV file: %s"), *WavPath);
		return false;
	}

	if (!WaveInfo.SampleDataStart || WaveInfo.SampleDataSize <= 0)
	{
		OutErrorMessage = FString::Printf(TEXT("WAV file contains no sample data: %s"), *WavPath);
		return false;
	}

	return true;
}

USoundWaveProcedural* FLocalTTSWavLoader::LoadSoundWave(
	UObject* Outer,
	const FString& WavPath,
	FString& OutErrorMessage) const
{
	TArray<uint8> WavBytes;
	if (!LocalTTSWavLoader::LoadWavBytes(WavPath, WavBytes, OutErrorMessage))
	{
		return nullptr;
	}

	FWaveModInfo WaveInfo;
	if (!WaveInfo.ReadWaveInfo(WavBytes.GetData(), WavBytes.Num()))
	{
		OutErrorMessage = FString::Printf(TEXT("Invalid WAV file: %s"), *WavPath);
		return nullptr;
	}

	if (!WaveInfo.SampleDataStart || WaveInfo.SampleDataSize <= 0)
	{
		OutErrorMessage = FString::Printf(TEXT("WAV file contains no sample data: %s"), *WavPath);
		return nullptr;
	}

	const int32 NumChannels = *WaveInfo.pChannels;
	const int32 SampleRate = *WaveInfo.pSamplesPerSec;
	const int32 BitsPerSample = *WaveInfo.pBitsPerSample;
	const int32 BytesPerSample = FMath::Max(1, BitsPerSample / 8);
	const int32 BlockAlign = FMath::Max(1, NumChannels * BytesPerSample);
	const float DurationSeconds = static_cast<float>(WaveInfo.SampleDataSize) / static_cast<float>(SampleRate * BlockAlign);

	USoundWaveProcedural* SoundWave = NewObject<USoundWaveProcedural>(Outer ? Outer : GetTransientPackage());
	if (!SoundWave)
	{
		OutErrorMessage = TEXT("Failed to allocate procedural sound wave.");
		return nullptr;
	}

	SoundWave->SetSampleRate(SampleRate);
	SoundWave->NumChannels = NumChannels;
	SoundWave->Duration = DurationSeconds;
	SoundWave->SoundGroup = SOUNDGROUP_Default;
	SoundWave->bLooping = false;
	SoundWave->SampleByteSize = BytesPerSample;
	SoundWave->QueueAudio(WaveInfo.SampleDataStart, WaveInfo.SampleDataSize);

	return SoundWave;
}
