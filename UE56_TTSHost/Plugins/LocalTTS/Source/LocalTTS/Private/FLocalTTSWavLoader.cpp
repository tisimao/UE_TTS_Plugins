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
			OutErrorMessage = FString::Printf(TEXT("WAV 文件不存在：%s"), *WavPath);
			return false;
		}

		if (!FFileHelper::LoadFileToArray(OutBytes, *WavPath))
		{
			OutErrorMessage = FString::Printf(TEXT("读取 WAV 文件失败：%s"), *WavPath);
			return false;
		}

		return true;
	}

	static bool ValidateWaveInfoForPlayback(
		const FString& WavPath,
		const FWaveModInfo& WaveInfo,
		FString& OutErrorMessage)
	{
		if (!WaveInfo.SampleDataStart || WaveInfo.SampleDataSize <= 0)
		{
			OutErrorMessage = FString::Printf(TEXT("WAV 文件不包含有效采样数据：%s"), *WavPath);
			return false;
		}

		if (!WaveInfo.pFormatTag || *WaveInfo.pFormatTag != FWaveModInfo::WAVE_INFO_FORMAT_PCM)
		{
			OutErrorMessage = FString::Printf(
				TEXT("LocalTTS 播放不支持该 WAV 格式：%s。当前仅支持 16 位 PCM WAV。"),
				*WavPath);
			return false;
		}

		if (!WaveInfo.pBitsPerSample || *WaveInfo.pBitsPerSample != 16)
		{
			OutErrorMessage = FString::Printf(
				TEXT("LocalTTS 播放不支持该 WAV 位深：%s。期望为 16 位 PCM 音频。"),
				*WavPath);
			return false;
		}

		if (!WaveInfo.pChannels || *WaveInfo.pChannels < 1 || *WaveInfo.pChannels > 2)
		{
			OutErrorMessage = FString::Printf(
				TEXT("LocalTTS 播放不支持该声道数：%s。当前仅支持单声道或双声道 WAV。"),
				*WavPath);
			return false;
		}

		if (!WaveInfo.pSamplesPerSec || *WaveInfo.pSamplesPerSec <= 0)
		{
			OutErrorMessage = FString::Printf(TEXT("WAV 文件采样率无效：%s"), *WavPath);
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
		OutErrorMessage = FString::Printf(TEXT("无效的 WAV 文件：%s"), *WavPath);
		return false;
	}

	return LocalTTSWavLoader::ValidateWaveInfoForPlayback(WavPath, WaveInfo, OutErrorMessage);
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
		OutErrorMessage = FString::Printf(TEXT("无效的 WAV 文件：%s"), *WavPath);
		return nullptr;
	}

	// The runtime playback path currently targets the OmniVoice output we generate in this project:
	// uncompressed 16-bit PCM WAV data loaded into a procedural sound wave.
	if (!LocalTTSWavLoader::ValidateWaveInfoForPlayback(WavPath, WaveInfo, OutErrorMessage))
	{
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
		OutErrorMessage = TEXT("创建 Procedural SoundWave 失败。");
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
