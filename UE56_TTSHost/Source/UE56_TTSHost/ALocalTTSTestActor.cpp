// Copyright Epic Games, Inc. All Rights Reserved.

#include "ALocalTTSTestActor.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Misc/Paths.h"
#include "TimerManager.h"

#include "ULocalTTSSettings.h"
#include "ULocalTTSSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogLocalTTSTestActor, Log, All);

namespace LocalTTSTestActorText
{
	static FString ToYesNo(const bool bValue)
	{
		return bValue ? TEXT("是") : TEXT("否");
	}

	static FString ToReadableStatus(const FString& Status)
	{
		if (Status.Equals(TEXT("ready"), ESearchCase::IgnoreCase))
		{
			return TEXT("就绪");
		}

		if (Status.Equals(TEXT("starting"), ESearchCase::IgnoreCase))
		{
			return TEXT("启动中");
		}

		if (Status.Equals(TEXT("busy"), ESearchCase::IgnoreCase))
		{
			return TEXT("忙碌");
		}

		if (Status.Equals(TEXT("stopped"), ESearchCase::IgnoreCase))
		{
			return TEXT("已停止");
		}

		if (Status.Equals(TEXT("error"), ESearchCase::IgnoreCase))
		{
			return TEXT("错误");
		}

		return Status.IsEmpty() ? TEXT("未知") : Status;
	}

	static const TCHAR* PlayModeOnlyMessage()
	{
		return TEXT("LocalTTS 示例 Actor 仅能在 Play 模式下使用。请先启动 PIE，让 GameInstance 和 LocalTTS 子系统创建完成。");
	}

	static const TCHAR* SubsystemUnavailableBeforeStartMessage()
	{
		return TEXT("在示例流程准备启动服务前，LocalTTS 子系统已不可用。");
	}

	static const TCHAR* BusyWaitingForGenerateMessage()
	{
		return TEXT("当前正在生成语音，请等待本次请求完成。生成完成后可直接播放最近一次生成的 WAV。");
	}
}

ALocalTTSTestActor::ALocalTTSTestActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SpeakRequest.Text = TEXT("你好，这是一次 UE 本地语音测试。");
	SpeakRequest.Mode = TEXT("auto");
	SpeakRequest.LanguageId = TEXT("zh");
	SpeakRequest.Speed = 1.0f;
}

void ALocalTTSTestActor::BeginPlay()
{
	Super::BeginPlay();

	if (bCheckHealthOnBeginPlay)
	{
		CheckLocalTTSHealth();
	}

	if (bStartServiceOnBeginPlay)
	{
		StartLocalTTSService();
	}

	if (bSpeakOnBeginPlay)
	{
		if (UWorld* World = GetWorld())
		{
			FTimerHandle TimerHandle;
			World->GetTimerManager().SetTimer(
				TimerHandle,
				this,
				&ALocalTTSTestActor::ExecuteDeferredSpeak,
				FMath::Max(0.1f, AutoSpeakDelaySeconds),
				false);
		}
	}
}

void ALocalTTSTestActor::CheckLocalTTSHealth()
{
	ULocalTTSSubsystem* Subsystem = ResolveSubsystem();
	if (!Subsystem)
	{
		StoreError(LocalTTSTestActorText::PlayModeOnlyMessage());
		return;
	}

	TWeakObjectPtr<ALocalTTSTestActor> WeakThis(this);
	Subsystem->CheckHealth(
		[WeakThis](const FLocalTTSHealthResponse& Response)
		{
			if (!WeakThis.IsValid())
			{
				return;
			}

			WeakThis->LastHealthResponse = Response;
			WeakThis->LastErrorMessage.Reset();
			WeakThis->LastHealthSummary = FString::Printf(
				TEXT("成功=%s 状态=%s 模型=%s"),
				*LocalTTSTestActorText::ToYesNo(Response.bOk),
				*LocalTTSTestActorText::ToReadableStatus(Response.Status),
				*Response.Model);
			UE_LOG(
				LogLocalTTSTestActor,
				Log,
				TEXT("LocalTTS health ok=%s status=%s model=%s"),
				Response.bOk ? TEXT("true") : TEXT("false"),
				*Response.Status,
				*Response.Model);
		},
		[WeakThis](const FString& ErrorMessage)
		{
			if (WeakThis.IsValid())
			{
				WeakThis->StoreError(ErrorMessage);
			}
		});
}

void ALocalTTSTestActor::StartLocalTTSService()
{
	ULocalTTSSubsystem* Subsystem = ResolveSubsystem();
	if (!Subsystem)
	{
		StoreError(LocalTTSTestActorText::PlayModeOnlyMessage());
		return;
	}

	FString ErrorMessage;
	if (!Subsystem->StartService(ErrorMessage))
	{
		StoreError(ErrorMessage);
		return;
	}

	LastErrorMessage.Reset();
	UE_LOG(LogLocalTTSTestActor, Log, TEXT("Requested LocalTTS service startup."));
}

void ALocalTTSTestActor::SpeakLocalTTSTest()
{
	ULocalTTSSubsystem* Subsystem = ResolveSubsystem();
	if (!Subsystem)
	{
		StoreError(LocalTTSTestActorText::PlayModeOnlyMessage());
		return;
	}

	if (Subsystem->IsTTSRequestInFlight())
	{
		if (bGenerateOnlyRequestInFlight)
		{
			bPlayCachedSpeechAfterGenerate = true;
			LastErrorMessage = TEXT("当前正在执行“仅生成示例 WAV”。生成完成后会自动播放这次生成的音频。");
		}
		else
		{
			LastErrorMessage = LocalTTSTestActorText::BusyWaitingForGenerateMessage();
		}
		return;
	}

	if (Subsystem->IsSpeaking())
	{
		LastErrorMessage = TEXT("当前已有音频正在播放。如需重新播放，请先点击“停止播放”。");
		return;
	}

	if (HasPlayableGeneratedAudio())
	{
		PlayCachedSpeech();
		return;
	}

	BeginSpeakFlow();
}

void ALocalTTSTestActor::GenerateLocalTTSTest()
{
	ULocalTTSSubsystem* Subsystem = ResolveSubsystem();
	if (!Subsystem)
	{
		StoreError(LocalTTSTestActorText::PlayModeOnlyMessage());
		return;
	}

	BeginGenerateFlow();
}

void ALocalTTSTestActor::StopLocalTTSSpeaking()
{
	ULocalTTSSubsystem* Subsystem = ResolveSubsystem();
	if (!Subsystem)
	{
		StoreError(LocalTTSTestActorText::PlayModeOnlyMessage());
		return;
	}

	Subsystem->StopSpeaking();
	LastErrorMessage.Reset();
	UE_LOG(LogLocalTTSTestActor, Log, TEXT("Requested LocalTTS playback stop."));
}

ULocalTTSSubsystem* ALocalTTSTestActor::ResolveSubsystem() const
{
	if (!GEngine)
	{
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return nullptr;
	}

	UGameInstance* GameInstance = World->GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<ULocalTTSSubsystem>() : nullptr;
}

bool ALocalTTSTestActor::HasPlayableGeneratedAudio() const
{
	return LastTTSResponse.bOk
		&& !LastTTSResponse.WavPath.IsEmpty()
		&& FPaths::FileExists(LastTTSResponse.WavPath);
}

void ALocalTTSTestActor::PlayCachedSpeech()
{
	ULocalTTSSubsystem* Subsystem = ResolveSubsystem();
	if (!Subsystem)
	{
		StoreError(TEXT("在播放最近一次生成的音频前，LocalTTS 子系统已不可用。"));
		return;
	}

	if (!HasPlayableGeneratedAudio())
	{
		StoreError(TEXT("当前没有可直接播放的已生成 WAV。请先点击“仅生成示例 WAV”，或直接点击“播放示例语音”重新生成并播放。"));
		return;
	}

	LastErrorMessage = TEXT("正在播放最近一次生成的音频。");
	Subsystem->PlaySpeech(
		this,
		LastTTSResponse,
		[this]()
		{
			LastErrorMessage.Reset();
			UE_LOG(LogLocalTTSTestActor, Log, TEXT("LocalTTS cached audio playback started."));
		},
		[this]()
		{
			UE_LOG(LogLocalTTSTestActor, Log, TEXT("LocalTTS cached audio playback finished."));
		},
		[this](const FString& ErrorMessage)
		{
			StoreError(ErrorMessage);
		});
}

void ALocalTTSTestActor::ExecuteDeferredSpeak()
{
	BeginSpeakFlow();
}

void ALocalTTSTestActor::BeginSpeakFlow()
{
	ULocalTTSSubsystem* Subsystem = ResolveSubsystem();
	if (!Subsystem)
	{
		StoreError(LocalTTSTestActorText::PlayModeOnlyMessage());
		return;
	}

	TWeakObjectPtr<ALocalTTSTestActor> WeakThis(this);
	Subsystem->CheckHealth(
		[WeakThis](const FLocalTTSHealthResponse& Response)
		{
			if (!WeakThis.IsValid())
			{
				return;
			}

			WeakThis->LastHealthResponse = Response;
			WeakThis->LastHealthSummary = FString::Printf(
				TEXT("成功=%s 状态=%s 模型=%s"),
				*LocalTTSTestActorText::ToYesNo(Response.bOk),
				*LocalTTSTestActorText::ToReadableStatus(Response.Status),
				*Response.Model);
			if (Response.bOk && Response.Status.Equals(TEXT("ready"), ESearchCase::IgnoreCase))
			{
				WeakThis->ExecuteSpeakRequest();
				return;
			}

			FString ErrorMessage;
			ULocalTTSSubsystem* Subsystem = WeakThis->ResolveSubsystem();
			if (!Subsystem)
			{
				WeakThis->StoreError(LocalTTSTestActorText::SubsystemUnavailableBeforeStartMessage());
				return;
			}

			if (!Subsystem->StartService(ErrorMessage))
			{
				WeakThis->StoreError(ErrorMessage);
				return;
			}

			WeakThis->BeginHealthPolling();
		},
		[WeakThis](const FString&)
		{
			if (!WeakThis.IsValid())
			{
				return;
			}

			FString ErrorMessage;
			ULocalTTSSubsystem* Subsystem = WeakThis->ResolveSubsystem();
			if (!Subsystem)
			{
				WeakThis->StoreError(LocalTTSTestActorText::SubsystemUnavailableBeforeStartMessage());
				return;
			}

			if (!Subsystem->StartService(ErrorMessage))
			{
				WeakThis->StoreError(ErrorMessage);
				return;
			}

			WeakThis->BeginHealthPolling();
		});
}

void ALocalTTSTestActor::BeginGenerateFlow()
{
	ULocalTTSSubsystem* Subsystem = ResolveSubsystem();
	if (!Subsystem)
	{
		StoreError(TEXT("LocalTTS 子系统不可用。请在 Play 模式下运行该 Actor，确保 GameInstance 已创建。"));
		return;
	}

	TWeakObjectPtr<ALocalTTSTestActor> WeakThis(this);
	Subsystem->CheckHealth(
		[WeakThis](const FLocalTTSHealthResponse& Response)
		{
			if (!WeakThis.IsValid())
			{
				return;
			}

			WeakThis->LastHealthResponse = Response;
			WeakThis->LastHealthSummary = FString::Printf(
				TEXT("成功=%s 状态=%s 模型=%s"),
				*LocalTTSTestActorText::ToYesNo(Response.bOk),
				*LocalTTSTestActorText::ToReadableStatus(Response.Status),
				*Response.Model);
			if (Response.bOk && Response.Status.Equals(TEXT("ready"), ESearchCase::IgnoreCase))
			{
				WeakThis->ExecuteGenerateRequest();
				return;
			}

			FString ErrorMessage;
			ULocalTTSSubsystem* Subsystem = WeakThis->ResolveSubsystem();
			if (!Subsystem)
			{
				WeakThis->StoreError(LocalTTSTestActorText::SubsystemUnavailableBeforeStartMessage());
				return;
			}

			if (!Subsystem->StartService(ErrorMessage))
			{
				WeakThis->StoreError(ErrorMessage);
				return;
			}

			WeakThis->BeginHealthPollingForGenerate();
		},
		[WeakThis](const FString&)
		{
			if (!WeakThis.IsValid())
			{
				return;
			}

			FString ErrorMessage;
			ULocalTTSSubsystem* Subsystem = WeakThis->ResolveSubsystem();
			if (!Subsystem)
			{
				WeakThis->StoreError(LocalTTSTestActorText::SubsystemUnavailableBeforeStartMessage());
				return;
			}

			if (!Subsystem->StartService(ErrorMessage))
			{
				WeakThis->StoreError(ErrorMessage);
				return;
			}

			WeakThis->BeginHealthPollingForGenerate();
		});
}

void ALocalTTSTestActor::ExecuteSpeakRequest()
{
	ULocalTTSSubsystem* Subsystem = ResolveSubsystem();
	if (!Subsystem)
	{
		StoreError(TEXT("在发送示例语音请求前，LocalTTS 子系统已不可用。"));
		return;
	}

	TWeakObjectPtr<ALocalTTSTestActor> WeakThis(this);
	Subsystem->SpeakText(
		SpeakRequest,
		[WeakThis](const FLocalTTSTTSResponse& Response)
		{
			if (!WeakThis.IsValid())
			{
				return;
			}

			WeakThis->bGenerateOnlyRequestInFlight = false;
			WeakThis->bPlayCachedSpeechAfterGenerate = false;
			WeakThis->LastTTSResponse = Response;
			WeakThis->LastGeneratedWavPath = Response.WavPath;
			WeakThis->LastTTSSummary = FString::Printf(
				TEXT("成功=%s 请求ID=%s WAV=%s 时长毫秒=%d"),
				*LocalTTSTestActorText::ToYesNo(Response.bOk),
				*Response.RequestId,
				*Response.WavPath,
				Response.DurationMs);
			WeakThis->LastErrorMessage.Reset();

			ULocalTTSSubsystem* Subsystem = WeakThis->ResolveSubsystem();
			if (!Subsystem)
			{
				WeakThis->StoreError(TEXT("在开始示例播放前，LocalTTS 子系统已不可用。"));
				return;
			}

			Subsystem->PlaySpeech(
				WeakThis.Get(),
				Response,
				[WeakThis]()
				{
					if (WeakThis.IsValid())
					{
						UE_LOG(LogLocalTTSTestActor, Log, TEXT("LocalTTS audio playback started."));
					}
				},
				[WeakThis]()
				{
					if (WeakThis.IsValid())
					{
						UE_LOG(LogLocalTTSTestActor, Log, TEXT("LocalTTS audio playback finished."));
					}
				},
				[WeakThis](const FString& ErrorMessage)
				{
					if (WeakThis.IsValid())
					{
						WeakThis->StoreError(ErrorMessage);
					}
				});
		},
		[WeakThis](const FString& ErrorMessage)
		{
			if (WeakThis.IsValid())
			{
				WeakThis->bGenerateOnlyRequestInFlight = false;
				WeakThis->bPlayCachedSpeechAfterGenerate = false;
				WeakThis->StoreError(ErrorMessage);
			}
		});
}

void ALocalTTSTestActor::ExecuteGenerateRequest()
{
	ULocalTTSSubsystem* Subsystem = ResolveSubsystem();
	if (!Subsystem)
	{
		StoreError(TEXT("在发送示例生成请求前，LocalTTS 子系统已不可用。"));
		return;
	}

	bGenerateOnlyRequestInFlight = true;
	bPlayCachedSpeechAfterGenerate = false;

	TWeakObjectPtr<ALocalTTSTestActor> WeakThis(this);
	Subsystem->SpeakText(
		SpeakRequest,
		[WeakThis](const FLocalTTSTTSResponse& Response)
		{
			if (!WeakThis.IsValid())
			{
				return;
			}

			const bool bShouldAutoPlayGeneratedAudio = WeakThis->bPlayCachedSpeechAfterGenerate;
			WeakThis->bGenerateOnlyRequestInFlight = false;
			WeakThis->bPlayCachedSpeechAfterGenerate = false;
			WeakThis->LastTTSResponse = Response;
			WeakThis->LastSpeechEvent = FLocalTTSSpeechEvent::FromRequestAndResponse(WeakThis->SpeakRequest, Response);
			WeakThis->LastGeneratedWavPath = Response.WavPath;
			WeakThis->LastTTSSummary = FString::Printf(
				TEXT("成功=%s 请求ID=%s WAV=%s 时长毫秒=%d"),
				*LocalTTSTestActorText::ToYesNo(Response.bOk),
				*Response.RequestId,
				*Response.WavPath,
				Response.DurationMs);
			WeakThis->LastSpeechEventSummary = FString::Printf(
				TEXT("请求ID=%s 文本=%s WAV=%s 时长秒=%.2f 口型帧=%d 情绪帧=%d"),
				*WeakThis->LastSpeechEvent.RequestId,
				*WeakThis->LastSpeechEvent.Text,
				*WeakThis->LastSpeechEvent.WavPath,
				WeakThis->LastSpeechEvent.DurationSeconds,
				WeakThis->LastSpeechEvent.VisemeFrames.Num(),
				WeakThis->LastSpeechEvent.EmotionFrames.Num());
			WeakThis->LastErrorMessage.Reset();
			UE_LOG(
				LogLocalTTSTestActor,
				Log,
				TEXT("LocalTTS generated speech event request_id=%s wav=%s"),
				*WeakThis->LastSpeechEvent.RequestId,
				*WeakThis->LastSpeechEvent.WavPath);

			if (bShouldAutoPlayGeneratedAudio)
			{
				WeakThis->PlayCachedSpeech();
			}
		},
		[WeakThis](const FString& ErrorMessage)
		{
			if (WeakThis.IsValid())
			{
				WeakThis->bGenerateOnlyRequestInFlight = false;
				WeakThis->bPlayCachedSpeechAfterGenerate = false;
				WeakThis->StoreError(ErrorMessage);
			}
		});
}

void ALocalTTSTestActor::BeginHealthPolling()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		StoreError(TEXT("示例 Actor 在等待 LocalTTS 就绪时，无法解析当前 World。"));
		return;
	}

	const ULocalTTSSettings* Settings = GetDefault<ULocalTTSSettings>();
	const int32 MaxHealthPollCount = Settings ? Settings->MaxHealthPollCount : 30;
	const float HealthPollIntervalSeconds = Settings ? Settings->HealthPollIntervalSeconds : 1.0f;

	RemainingHealthPollCount = FMath::Max(1, MaxHealthPollCount);
	World->GetTimerManager().SetTimer(
		HealthPollTimerHandle,
		this,
		&ALocalTTSTestActor::PollServiceHealth,
		FMath::Max(0.1f, HealthPollIntervalSeconds),
		true);
}

void ALocalTTSTestActor::PollServiceHealth()
{
	ULocalTTSSubsystem* Subsystem = ResolveSubsystem();
	if (!Subsystem)
	{
		StoreError(TEXT("示例 Actor 在等待服务就绪时，LocalTTS 子系统已不可用。"));
		return;
	}

	if (RemainingHealthPollCount <= 0)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(HealthPollTimerHandle);
		}
		StoreError(TEXT("等待 LocalTTS 进入“就绪”状态超时。请检查服务控制台、Project Settings > Plugins > LocalTTS，以及是否已执行 Setup_TTS_Service.bat。"));
		return;
	}

	--RemainingHealthPollCount;

	TWeakObjectPtr<ALocalTTSTestActor> WeakThis(this);
	Subsystem->CheckHealth(
		[WeakThis](const FLocalTTSHealthResponse& Response)
		{
			if (!WeakThis.IsValid())
			{
				return;
			}

			WeakThis->LastHealthResponse = Response;
			WeakThis->LastHealthSummary = FString::Printf(
				TEXT("成功=%s 状态=%s 模型=%s"),
				*LocalTTSTestActorText::ToYesNo(Response.bOk),
				*LocalTTSTestActorText::ToReadableStatus(Response.Status),
				*Response.Model);
			if (!Response.bOk || !Response.Status.Equals(TEXT("ready"), ESearchCase::IgnoreCase))
			{
				return;
			}

			if (UWorld* World = WeakThis->GetWorld())
			{
				World->GetTimerManager().ClearTimer(WeakThis->HealthPollTimerHandle);
			}

			WeakThis->ExecuteSpeakRequest();
		},
		[](const FString&)
		{
		});
}

void ALocalTTSTestActor::BeginHealthPollingForGenerate()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		StoreError(TEXT("示例 Actor 在等待 LocalTTS 就绪时，无法解析当前 World。"));
		return;
	}

	const ULocalTTSSettings* Settings = GetDefault<ULocalTTSSettings>();
	const int32 MaxHealthPollCount = Settings ? Settings->MaxHealthPollCount : 30;
	const float HealthPollIntervalSeconds = Settings ? Settings->HealthPollIntervalSeconds : 1.0f;

	RemainingHealthPollCount = FMath::Max(1, MaxHealthPollCount);
	World->GetTimerManager().SetTimer(
		HealthPollTimerHandle,
		this,
		&ALocalTTSTestActor::PollServiceHealthForGenerate,
		FMath::Max(0.1f, HealthPollIntervalSeconds),
		true);
}

void ALocalTTSTestActor::PollServiceHealthForGenerate()
{
	ULocalTTSSubsystem* Subsystem = ResolveSubsystem();
	if (!Subsystem)
	{
		StoreError(TEXT("示例 Actor 在等待服务就绪时，LocalTTS 子系统已不可用。"));
		return;
	}

	if (RemainingHealthPollCount <= 0)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(HealthPollTimerHandle);
		}
		StoreError(TEXT("等待 LocalTTS 进入“就绪”状态超时。请检查服务控制台、Project Settings > Plugins > LocalTTS，以及是否已执行 Setup_TTS_Service.bat。"));
		return;
	}

	--RemainingHealthPollCount;

	TWeakObjectPtr<ALocalTTSTestActor> WeakThis(this);
	Subsystem->CheckHealth(
		[WeakThis](const FLocalTTSHealthResponse& Response)
		{
			if (!WeakThis.IsValid())
			{
				return;
			}

			WeakThis->LastHealthResponse = Response;
			WeakThis->LastHealthSummary = FString::Printf(
				TEXT("成功=%s 状态=%s 模型=%s"),
				*LocalTTSTestActorText::ToYesNo(Response.bOk),
				*LocalTTSTestActorText::ToReadableStatus(Response.Status),
				*Response.Model);
			if (!Response.bOk || !Response.Status.Equals(TEXT("ready"), ESearchCase::IgnoreCase))
			{
				return;
			}

			if (UWorld* World = WeakThis->GetWorld())
			{
				World->GetTimerManager().ClearTimer(WeakThis->HealthPollTimerHandle);
			}

			WeakThis->ExecuteGenerateRequest();
		},
		[](const FString&)
		{
		});
}

void ALocalTTSTestActor::StoreError(const FString& ErrorMessage)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HealthPollTimerHandle);
	}

	LastErrorMessage = ErrorMessage;
	UE_LOG(LogLocalTTSTestActor, Error, TEXT("%s"), *ErrorMessage);
}
