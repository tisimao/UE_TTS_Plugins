// Copyright Epic Games, Inc. All Rights Reserved.

#include "FLocalTTSHttpClient.h"

#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace LocalTTSHttpClient
{
	static FString BuildHealthUrl(const FString& BaseUrl)
	{
		FString NormalizedBaseUrl = BaseUrl;
		NormalizedBaseUrl.RemoveFromEnd(TEXT("/"));
		return FString::Printf(TEXT("%s/health"), *NormalizedBaseUrl);
	}

	static FString BuildTTSUrl(const FString& BaseUrl)
	{
		FString NormalizedBaseUrl = BaseUrl;
		NormalizedBaseUrl.RemoveFromEnd(TEXT("/"));
		return FString::Printf(TEXT("%s/tts"), *NormalizedBaseUrl);
	}

	static bool DeserializeJsonObject(
		const FString& JsonString,
		TSharedPtr<FJsonObject>& OutJsonObject)
	{
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
		return FJsonSerializer::Deserialize(Reader, OutJsonObject) && OutJsonObject.IsValid();
	}

	static bool ParseHealthResponse(
		const FString& JsonString,
		FLocalTTSHealthResponse& OutHealthResponse)
	{
		TSharedPtr<FJsonObject> JsonObject;
		if (!DeserializeJsonObject(JsonString, JsonObject))
		{
			return false;
		}

		if (!JsonObject->TryGetBoolField(TEXT("ok"), OutHealthResponse.bOk))
		{
			return false;
		}

		JsonObject->TryGetStringField(TEXT("service"), OutHealthResponse.Service);
		JsonObject->TryGetStringField(TEXT("status"), OutHealthResponse.Status);
		JsonObject->TryGetStringField(TEXT("model"), OutHealthResponse.Model);

		OutHealthResponse.SupportedModes.Reset();
		const TArray<TSharedPtr<FJsonValue>>* SupportedModes = nullptr;
		if (JsonObject->TryGetArrayField(TEXT("supported_modes"), SupportedModes))
		{
			for (const TSharedPtr<FJsonValue>& Value : *SupportedModes)
			{
				FString Mode;
				if (Value.IsValid() && Value->TryGetString(Mode))
				{
					OutHealthResponse.SupportedModes.Add(Mode);
				}
			}
		}

		return true;
	}

	static bool ParseTTSResponse(
		const FString& JsonString,
		FLocalTTSTTSResponse& OutTTSResponse)
	{
		TSharedPtr<FJsonObject> JsonObject;
		if (!DeserializeJsonObject(JsonString, JsonObject))
		{
			return false;
		}

		if (!JsonObject->TryGetBoolField(TEXT("ok"), OutTTSResponse.bOk))
		{
			return false;
		}

		JsonObject->TryGetStringField(TEXT("request_id"), OutTTSResponse.RequestId);
		JsonObject->TryGetStringField(TEXT("mode"), OutTTSResponse.Mode);
		JsonObject->TryGetStringField(TEXT("wav_path"), OutTTSResponse.WavPath);

		double SampleRate = 0.0;
		if (JsonObject->TryGetNumberField(TEXT("sample_rate"), SampleRate))
		{
			OutTTSResponse.SampleRate = static_cast<int32>(SampleRate);
		}

		double DurationMs = 0.0;
		if (JsonObject->TryGetNumberField(TEXT("duration_ms"), DurationMs))
		{
			OutTTSResponse.DurationMs = static_cast<int32>(DurationMs);
		}

		JsonObject->TryGetStringField(TEXT("error_code"), OutTTSResponse.ErrorCode);
		JsonObject->TryGetStringField(TEXT("error_message"), OutTTSResponse.ErrorMessage);
		return true;
	}

	static FString SerializeSpeakRequest(const FLocalTTSSpeakRequest& SpeakRequest)
	{
		TSharedRef<FJsonObject> JsonObject = MakeShared<FJsonObject>();
		JsonObject->SetStringField(TEXT("text"), SpeakRequest.Text);
		JsonObject->SetStringField(TEXT("mode"), SpeakRequest.Mode.IsEmpty() ? TEXT("auto") : SpeakRequest.Mode);

		if (!SpeakRequest.LanguageId.IsEmpty())
		{
			JsonObject->SetStringField(TEXT("language_id"), SpeakRequest.LanguageId);
		}
		if (!SpeakRequest.ReferenceAudioPath.IsEmpty())
		{
			JsonObject->SetStringField(TEXT("ref_audio"), SpeakRequest.ReferenceAudioPath);
		}
		if (!SpeakRequest.ReferenceText.IsEmpty())
		{
			JsonObject->SetStringField(TEXT("ref_text"), SpeakRequest.ReferenceText);
		}
		if (!SpeakRequest.Instruct.IsEmpty())
		{
			JsonObject->SetStringField(TEXT("instruct"), SpeakRequest.Instruct);
		}
		if (SpeakRequest.Duration > 0.0f)
		{
			JsonObject->SetNumberField(TEXT("duration"), SpeakRequest.Duration);
		}
		if (SpeakRequest.Speed > 0.0f)
		{
			JsonObject->SetNumberField(TEXT("speed"), SpeakRequest.Speed);
		}

		FString RequestBody;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
		FJsonSerializer::Serialize(JsonObject, Writer);
		return RequestBody;
	}
}

void FLocalTTSHttpClient::GetHealth(
	const FString& BaseUrl,
	FOnHealthSuccess&& OnSuccess,
	FOnRequestFailure&& OnFailure) const
{
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(LocalTTSHttpClient::BuildHealthUrl(BaseUrl));
	Request->SetVerb(TEXT("GET"));
	Request->SetHeader(TEXT("Accept"), TEXT("application/json"));

	Request->OnProcessRequestComplete().BindLambda(
		[OnSuccess = MoveTemp(OnSuccess), OnFailure = MoveTemp(OnFailure)](
			FHttpRequestPtr HttpRequest,
			FHttpResponsePtr HttpResponse,
			bool bWasSuccessful) mutable
		{
			if (!bWasSuccessful || !HttpResponse.IsValid())
			{
				OnFailure(TEXT("LocalTTS service is unreachable."));
				return;
			}

			const int32 ResponseCode = HttpResponse->GetResponseCode();
			if (!EHttpResponseCodes::IsOk(ResponseCode))
			{
				OnFailure(FString::Printf(TEXT("LocalTTS health request failed. HTTP %d"), ResponseCode));
				return;
			}

			FLocalTTSHealthResponse HealthResponse;
			if (!LocalTTSHttpClient::ParseHealthResponse(
				HttpResponse->GetContentAsString(),
				HealthResponse))
			{
				OnFailure(TEXT("Failed to parse LocalTTS health response."));
				return;
			}

			OnSuccess(HealthResponse);
		});

	Request->ProcessRequest();
}

void FLocalTTSHttpClient::PostTTS(
	const FString& BaseUrl,
	const FLocalTTSSpeakRequest& SpeakRequest,
	FOnTTSSuccess&& OnSuccess,
	FOnRequestFailure&& OnFailure) const
{
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(LocalTTSHttpClient::BuildTTSUrl(BaseUrl));
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Accept"), TEXT("application/json"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json; charset=utf-8"));
	Request->SetContentAsString(LocalTTSHttpClient::SerializeSpeakRequest(SpeakRequest));

	Request->OnProcessRequestComplete().BindLambda(
		[OnSuccess = MoveTemp(OnSuccess), OnFailure = MoveTemp(OnFailure)](
			FHttpRequestPtr HttpRequest,
			FHttpResponsePtr HttpResponse,
			bool bWasSuccessful) mutable
		{
			if (!bWasSuccessful || !HttpResponse.IsValid())
			{
				OnFailure(TEXT("LocalTTS speech request failed because the service is unreachable."));
				return;
			}

			const int32 ResponseCode = HttpResponse->GetResponseCode();
			if (!EHttpResponseCodes::IsOk(ResponseCode))
			{
				OnFailure(FString::Printf(TEXT("LocalTTS speech request failed. HTTP %d"), ResponseCode));
				return;
			}

			FLocalTTSTTSResponse TTSResponse;
			if (!LocalTTSHttpClient::ParseTTSResponse(
				HttpResponse->GetContentAsString(),
				TTSResponse))
			{
				OnFailure(TEXT("Failed to parse LocalTTS speech response."));
				return;
			}

			if (!TTSResponse.bOk)
			{
				const FString ErrorMessage = TTSResponse.ErrorMessage.IsEmpty()
					? TEXT("LocalTTS returned an unknown error.")
					: TTSResponse.ErrorMessage;
				OnFailure(ErrorMessage);
				return;
			}

			OnSuccess(TTSResponse);
		});

	Request->ProcessRequest();
}
