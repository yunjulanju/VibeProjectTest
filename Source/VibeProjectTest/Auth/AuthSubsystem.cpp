#include "Auth/AuthSubsystem.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY(LogAuth);

void UAuthSubsystem::SendRequest(const FString& Path, const FString& Verb, const FString& Body, bool bAuth, FAuthCallback OnDone)
{
	// 보호 요청인데 토큰이 없으면 요청하지 않고 실패 통지.
	if (bAuth && AccessToken.IsEmpty())
	{
		UE_LOG(LogAuth, Warning, TEXT("[%s %s] access token 없음 — 로그인 먼저 필요"), *Verb, *Path);
		if (OnDone) { OnDone(false, 0, TEXT("no access token")); }
		return;
	}

	const FString Url = BaseUrl + Path;

	FHttpRequestRef Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(Url);
	Request->SetVerb(Verb);
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetHeader(TEXT("Accept"), TEXT("application/json"));
	if (bAuth)
	{
		Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *AccessToken));
	}
	if (!Body.IsEmpty())
	{
		Request->SetContentAsString(Body);
	}

	UE_LOG(LogAuth, Log, TEXT("→ %s %s"), *Verb, *Url);

	Request->OnProcessRequestComplete().BindLambda(
		[OnDone, Verb, Url](FHttpRequestPtr /*Req*/, FHttpResponsePtr Response, bool bWasSuccessful)
		{
			if (!bWasSuccessful || !Response.IsValid())
			{
				UE_LOG(LogAuth, Error, TEXT("✗ %s %s — 네트워크 실패 (서버 미기동?)"), *Verb, *Url);
				if (OnDone) { OnDone(false, 0, TEXT("network failure")); }
				return;
			}

			const int32 Code = Response->GetResponseCode();
			const FString Content = Response->GetContentAsString();
			const bool bOk = (Code >= 200 && Code < 300);

			if (bOk)
			{
				UE_LOG(LogAuth, Log, TEXT("✓ %s %s — %d %s"), *Verb, *Url, Code, *Content);
			}
			else
			{
				UE_LOG(LogAuth, Warning, TEXT("✗ %s %s — %d %s"), *Verb, *Url, Code, *Content);
			}

			if (OnDone) { OnDone(bOk, Code, Content); }
		});

	Request->ProcessRequest();
}

void UAuthSubsystem::Health(FAuthCallback OnDone)
{
	SendRequest(TEXT("/health"), TEXT("GET"), FString(), /*bAuth=*/false, MoveTemp(OnDone));
}

void UAuthSubsystem::Signup(const FString& Email, const FString& Username, const FString& Password, FAuthCallback OnDone)
{
	const TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
	Json->SetStringField(TEXT("email"), Email);
	Json->SetStringField(TEXT("username"), Username);
	Json->SetStringField(TEXT("password"), Password);

	FString Body;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Body);
	FJsonSerializer::Serialize(Json, Writer);

	SendRequest(TEXT("/auth/signup"), TEXT("POST"), Body, /*bAuth=*/false, MoveTemp(OnDone));
}

void UAuthSubsystem::Login(const FString& Email, const FString& Password, FAuthCallback OnDone)
{
	const TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
	Json->SetStringField(TEXT("email"), Email);
	Json->SetStringField(TEXT("password"), Password);

	FString Body;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Body);
	FJsonSerializer::Serialize(Json, Writer);

	// 성공 시 토큰을 저장한 뒤 원래 콜백으로 전달.
	FAuthCallback Wrapped = [this, OnDone](bool bSuccess, int32 Code, const FString& Content)
	{
		if (bSuccess)
		{
			TSharedPtr<FJsonObject> Parsed;
			const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Content);
			if (FJsonSerializer::Deserialize(Reader, Parsed) && Parsed.IsValid())
			{
				FString Token;
				if (Parsed->TryGetStringField(TEXT("access_token"), Token) && !Token.IsEmpty())
				{
					AccessToken = Token;
					UE_LOG(LogAuth, Log, TEXT("로그인 성공 — access token 저장됨"));
				}
				else
				{
					UE_LOG(LogAuth, Warning, TEXT("로그인 응답에 access_token 없음"));
				}
			}
			else
			{
				UE_LOG(LogAuth, Warning, TEXT("로그인 응답 JSON 파싱 실패"));
			}
		}
		if (OnDone) { OnDone(bSuccess, Code, Content); }
	};

	SendRequest(TEXT("/auth/login"), TEXT("POST"), Body, /*bAuth=*/false, MoveTemp(Wrapped));
}

void UAuthSubsystem::GetMe(FAuthCallback OnDone)
{
	SendRequest(TEXT("/users/me"), TEXT("GET"), FString(), /*bAuth=*/true, MoveTemp(OnDone));
}
void UAuthSubsystem::DeleteMe(FAuthCallback OnDone)
{
	FAuthCallback Wrapped = [this, OnDone](bool bSuccess, int32 Code, const FString& Content)
	{
		if (bSuccess)
		{
			ClearToken();
			UE_LOG(LogAuth, Log, TEXT("회원탈퇴 완료 — 토큰 클리어됨"));
		}
		if (OnDone) { OnDone(bSuccess, Code, Content); }
	};

	SendRequest(TEXT("/users/me"), TEXT("DELETE"), FString(), /*bAuth=*/true, MoveTemp(Wrapped));
}
