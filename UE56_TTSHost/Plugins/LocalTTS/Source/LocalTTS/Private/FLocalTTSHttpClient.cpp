// Copyright Epic Games, Inc. All Rights Reserved.

#include "FLocalTTSHttpClient.h"

#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

DEFINE_LOG_CATEGORY_STATIC(LogLocalTTSHttpClient, Log, All);

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
		const FString NormalizedMode = SpeakRequest.Mode.TrimStartAndEnd().IsEmpty()
			? TEXT("auto")
			: SpeakRequest.Mode.TrimStartAndEnd().ToLower();
		JsonObject->SetStringField(TEXT("text"), SpeakRequest.Text.TrimStartAndEnd());
		JsonObject->SetStringField(TEXT("mode"), NormalizedMode);

		const FString NormalizedLanguageId = SpeakRequest.LanguageId.TrimStartAndEnd().ToLower();
		if (!NormalizedLanguageId.IsEmpty())
		{
			JsonObject->SetStringField(TEXT("language_id"), NormalizedLanguageId);
		}
		const FString ReferenceAudioPath = SpeakRequest.ReferenceAudioPath.TrimStartAndEnd();
		if (!ReferenceAudioPath.IsEmpty())
		{
			JsonObject->SetStringField(TEXT("ref_audio"), ReferenceAudioPath);
		}
		const FString ReferenceText = SpeakRequest.ReferenceText.TrimStartAndEnd();
		if (!ReferenceText.IsEmpty())
		{
			JsonObject->SetStringField(TEXT("ref_text"), ReferenceText);
		}
		const FString Instruct = SpeakRequest.Instruct.TrimStartAndEnd();
		if (!Instruct.IsEmpty())
		{
			JsonObject->SetStringField(TEXT("instruct"), Instruct);
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
	const FString HealthUrl = LocalTTSHttpClient::BuildHealthUrl(BaseUrl);
	Request->SetURL(HealthUrl);
	Request->SetVerb(TEXT("GET"));
	Request->SetHeader(TEXT("Accept"), TEXT("application/json"));
	UE_LOG(LogLocalTTSHttpClient, Log, TEXT("GET %s"), *HealthUrl);

	Request->OnProcessRequestComplete().BindLambda(
		[OnSuccess = MoveTemp(OnSuccess), OnFailure = MoveTemp(OnFailure)](
			FHttpRequestPtr HttpRequest,
			FHttpResponsePtr HttpResponse,
			bool bWasSuccessful) mutable
		{
			if (!bWasSuccessful || !HttpResponse.IsValid())
			{
				OnFailure(ELocalTTSErrorCode::ServiceUnreachable, TEXT("LocalTTS service is unreachable."));
				return;
			}

			const int32 ResponseCode = HttpResponse->GetResponseCode();
			if (!EHttpResponseCodes::IsOk(ResponseCode))
			{
				OnFailure(ELocalTTSErrorCode::HttpError, FString::Printf(
					TEXT("LocalTTS health request failed. HTTP %d Body=%s"),
					ResponseCode,
					*HttpResponse->GetContentAsString()));
				return;
			}

			FLocalTTSHealthResponse HealthResponse;
			if (!LocalTTSHttpClient::ParseHealthResponse(
				HttpResponse->GetContentAsString(),
				HealthResponse))
			{
				OnFailure(ELocalTTSErrorCode::ParseError, TEXT("Failed to parse LocalTTS health response."));
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
	const FString TTSUrl = LocalTTSHttpClient::BuildTTSUrl(BaseUrl);
	const FString RequestBody = LocalTTSHttpClient::SerializeSpeakRequest(SpeakRequest);
	Request->SetURL(TTSUrl);
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Accept"), TEXT("application/json"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json; charset=utf-8"));
	Request->SetContentAsString(RequestBody);
	UE_LOG(LogLocalTTSHttpClient, Log, TEXT("POST %s Body=%s"), *TTSUrl, *RequestBody);

	Request->OnProcessRequestComplete().BindLambda(
		[OnSuccess = MoveTemp(OnSuccess), OnFailure = MoveTemp(OnFailure)](
			FHttpRequestPtr HttpRequest,
			FHttpResponsePtr HttpResponse,
			bool bWasSuccessful) mutable
		{
			if (!bWasSuccessful || !HttpResponse.IsValid())
			{
				OnFailure(ELocalTTSErrorCode::ServiceUnreachable, TEXT("LocalTTS speech request failed because the service is unreachable."));
				return;
			}

			const int32 ResponseCode = HttpResponse->GetResponseCode();
			if (!EHttpResponseCodes::IsOk(ResponseCode))
			{
				OnFailure(ELocalTTSErrorCode::HttpError, FString::Printf(
					TEXT("LocalTTS speech request failed. HTTP %d Body=%s"),
					ResponseCode,
					*HttpResponse->GetContentAsString()));
				return;
			}

			FLocalTTSTTSResponse TTSResponse;
			if (!LocalTTSHttpClient::ParseTTSResponse(
				HttpResponse->GetContentAsString(),
				TTSResponse))
			{
				OnFailure(ELocalTTSErrorCode::ParseError, TEXT("Failed to parse LocalTTS speech response."));
				return;
			}

			if (!TTSResponse.bOk)
			{
				const FString ErrorMessage = TTSResponse.ErrorMessage.IsEmpty()
					? TEXT("LocalTTS returned an unknown error.")
					: TTSResponse.ErrorMessage;
				OnFailure(ELocalTTSErrorCode::ServiceReturnedError, ErrorMessage);
				return;
			}

			OnSuccess(TTSResponse);
		});

	Request->ProcessRequest();
}
