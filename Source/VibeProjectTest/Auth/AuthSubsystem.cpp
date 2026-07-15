#include "Auth/AuthSubsystem.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"

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

// 엔드포인트 스텁 — Task 3~5에서 채움.
void UAuthSubsystem::Health(FAuthCallback OnDone) {}
void UAuthSubsystem::Signup(const FString& Email, const FString& Username, const FString& Password, FAuthCallback OnDone) {}
void UAuthSubsystem::Login(const FString& Email, const FString& Password, FAuthCallback OnDone) {}
void UAuthSubsystem::GetMe(FAuthCallback OnDone) {}
void UAuthSubsystem::DeleteMe(FAuthCallback OnDone) {}
