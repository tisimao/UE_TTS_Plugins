// Copyright Epic Games, Inc. All Rights Reserved.

#include "FLocalTTSServiceProcess.h"

#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformMisc.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "ULocalTTSSettings.h"

FLocalTTSServiceProcess::FLocalTTSServiceProcess()
{
}

FLocalTTSServiceProcess::~FLocalTTSServiceProcess()
{
	StopService();
}

bool FLocalTTSServiceProcess::IsRunning()
{
	return ProcessHandle.IsValid() && FPlatformProcess::IsProcRunning(ProcessHandle);
}

bool FLocalTTSServiceProcess::StartService(FString& OutErrorMessage)
{
	if (IsRunning())
	{
		return true;
	}

	const FString PythonExecutablePath = GetPythonExecutablePath();
	if (!FPaths::FileExists(PythonExecutablePath))
	{
		OutErrorMessage = FString::Printf(
			TEXT("LocalTTS 未找到 Python 可执行文件：%s。请先运行 Setup_TTS_Service.bat，或检查 Project Settings > Plugins > LocalTTS > Python Relative Path。"),
			*PythonExecutablePath);
		return false;
	}

	const FString RunServerScriptPath = GetRunServerScriptPath();
	if (!FPaths::FileExists(RunServerScriptPath))
	{
		OutErrorMessage = FString::Printf(
			TEXT("LocalTTS 未找到 run_server.py：%s。请检查 Project Settings > Plugins > LocalTTS > Service Relative Root 和 Run Server Script Name。"),
			*RunServerScriptPath);
		return false;
	}

	const FString ServiceRoot = GetServiceRoot();
	SetServiceEnvironment(ServiceRoot);
	const FString Arguments = TEXT("run_server.py");
	ProcessHandle = FPlatformProcess::CreateProc(
		*PythonExecutablePath,
		*Arguments,
		true,
		true,
		true,
		nullptr,
		0,
		*ServiceRoot,
		nullptr);

	if (!ProcessHandle.IsValid())
	{
		OutErrorMessage = TEXT("LocalTTS 启动服务进程失败。请确认 Python 环境已准备完成，并尝试手动运行 Services/tts_service/run_server.py 查看详情。");
		return false;
	}

	return true;
}

void FLocalTTSServiceProcess::StopService()
{
	if (!ProcessHandle.IsValid())
	{
		return;
	}

	FPlatformProcess::TerminateProc(ProcessHandle, true);
	FPlatformProcess::CloseProc(ProcessHandle);
	ProcessHandle.Reset();
}

FString FLocalTTSServiceProcess::GetServiceRoot() const
{
	const ULocalTTSSettings* Settings = GetDefault<ULocalTTSSettings>();
	const FString RelativeRoot = Settings
		? Settings->ServiceRelativeRoot
		: TEXT("Services/tts_service");

	TArray<FString> CandidateRoots;
	CandidateRoots.Add(FPaths::Combine(GetRepoRoot(), RelativeRoot));
	CandidateRoots.Add(FPaths::Combine(FPaths::ProjectDir(), RelativeRoot));
	CandidateRoots.Add(FPaths::Combine(FPlatformProcess::BaseDir(), RelativeRoot));
	CandidateRoots.Add(FPaths::Combine(FPlatformProcess::BaseDir(), TEXT(".."), RelativeRoot));
	CandidateRoots.Add(FPaths::Combine(FPlatformProcess::BaseDir(), TEXT(".."), TEXT(".."), RelativeRoot));
	CandidateRoots.Add(FPaths::Combine(FPlatformProcess::BaseDir(), TEXT(".."), TEXT(".."), TEXT(".."), RelativeRoot));

	const FString ScriptName = GetRunServerScriptName();
	for (const FString& CandidateRoot : CandidateRoots)
	{
		const FString FullCandidateRoot = FPaths::ConvertRelativePathToFull(CandidateRoot);
		if (FPaths::FileExists(FPaths::Combine(FullCandidateRoot, ScriptName)))
		{
			return FullCandidateRoot;
		}
	}

	return FPaths::ConvertRelativePathToFull(CandidateRoots[0]);
}

FString FLocalTTSServiceProcess::GetRepoRoot() const
{
	return FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), TEXT("..")));
}

FString FLocalTTSServiceProcess::GetPythonExecutablePath() const
{
	const ULocalTTSSettings* Settings = GetDefault<ULocalTTSSettings>();
	const FString RelativePath = Settings
		? Settings->PythonRelativePath
		: TEXT(".venv/Scripts/python.exe");
	return FPaths::Combine(GetServiceRoot(), RelativePath);
}

FString FLocalTTSServiceProcess::GetRunServerScriptName() const
{
	const ULocalTTSSettings* Settings = GetDefault<ULocalTTSSettings>();
	return Settings
		? Settings->RunServerScriptName
		: TEXT("run_server.py");
}

FString FLocalTTSServiceProcess::GetRunServerScriptPath() const
{
	return FPaths::Combine(GetServiceRoot(), GetRunServerScriptName());
}

void FLocalTTSServiceProcess::SetServiceEnvironment(const FString& ServiceRoot) const
{
	const FString ModelCacheDir = FPaths::Combine(ServiceRoot, TEXT("models"), TEXT("huggingface"));
	if (FPaths::DirectoryExists(ModelCacheDir))
	{
		FPlatformMisc::SetEnvironmentVar(TEXT("LOCAL_TTS_HF_CACHE_DIR"), *ModelCacheDir);
	}

	const FString WritableRoot = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("LocalTTS"));
	const FString CacheDir = FPaths::Combine(WritableRoot, TEXT("cache"));
	const FString LogDir = FPaths::Combine(WritableRoot, TEXT("logs"));
	IFileManager::Get().MakeDirectory(*CacheDir, true);
	IFileManager::Get().MakeDirectory(*LogDir, true);
	FPlatformMisc::SetEnvironmentVar(TEXT("LOCAL_TTS_CACHE_DIR"), *CacheDir);
	FPlatformMisc::SetEnvironmentVar(TEXT("LOCAL_TTS_LOG_DIR"), *LogDir);
}
