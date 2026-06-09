// Copyright Epic Games, Inc. All Rights Reserved.

#include "FLocalTTSServiceProcess.h"

#include "HAL/PlatformProcess.h"
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
		OutErrorMessage = FString::Printf(TEXT("Python executable not found: %s"), *PythonExecutablePath);
		return false;
	}

	const FString RunServerScriptPath = GetRunServerScriptPath();
	if (!FPaths::FileExists(RunServerScriptPath))
	{
		OutErrorMessage = FString::Printf(TEXT("run_server.py not found: %s"), *RunServerScriptPath);
		return false;
	}

	const FString ServiceRoot = GetServiceRoot();
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
		OutErrorMessage = TEXT("Failed to start LocalTTS service process.");
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

	return FPaths::ConvertRelativePathToFull(
		FPaths::Combine(GetRepoRoot(), RelativeRoot));
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

FString FLocalTTSServiceProcess::GetRunServerScriptPath() const
{
	const ULocalTTSSettings* Settings = GetDefault<ULocalTTSSettings>();
	const FString ScriptName = Settings
		? Settings->RunServerScriptName
		: TEXT("run_server.py");
	return FPaths::Combine(GetServiceRoot(), ScriptName);
}
