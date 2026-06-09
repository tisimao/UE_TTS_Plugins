// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class LOCALTTS_API FLocalTTSServiceProcess
{
public:
	FLocalTTSServiceProcess();
	~FLocalTTSServiceProcess();

	bool IsRunning();
	bool StartService(FString& OutErrorMessage);
	void StopService();

	FString GetServiceRoot() const;

private:
	FString GetRepoRoot() const;
	FString GetPythonExecutablePath() const;
	FString GetRunServerScriptPath() const;

	FProcHandle ProcessHandle;
};
