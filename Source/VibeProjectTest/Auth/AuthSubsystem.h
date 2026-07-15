#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/IHttpRequest.h"
#include "AuthSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogAuth, Log, All);

/** HTTP 응답 완료 콜백: 성공 여부, HTTP 상태코드, 응답 본문 */
using FAuthCallback = TFunction<void(bool bSuccess, int32 StatusCode, const FString& Content)>;

/**
 * FastAPI 인증 서버와 통신하는 서브시스템.
 * 로그인으로 받은 JWT access token을 보관했다가 보호된 요청에 Bearer로 첨부한다.
 */
UCLASS()
class VIBEPROJECTTEST_API UAuthSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// --- 설정/상태 ---
	void SetBaseUrl(const FString& InUrl) { BaseUrl = InUrl; }
	const FString& GetBaseUrl() const { return BaseUrl; }
	bool IsLoggedIn() const { return !AccessToken.IsEmpty(); }
	void ClearToken() { AccessToken.Reset(); }

	// --- 엔드포인트 (Task 3~5에서 구현) ---
	void Health(FAuthCallback OnDone = FAuthCallback());
	void Signup(const FString& Email, const FString& Username, const FString& Password, FAuthCallback OnDone = FAuthCallback());
	void Login(const FString& Email, const FString& Password, FAuthCallback OnDone = FAuthCallback());
	void GetMe(FAuthCallback OnDone = FAuthCallback());
	void DeleteMe(FAuthCallback OnDone = FAuthCallback());

private:
	/** 공통 요청 헬퍼: URL/Verb/헤더 구성, bAuth면 Bearer 첨부, 비동기 전송 후 콜백. */
	void SendRequest(const FString& Path, const FString& Verb, const FString& Body, bool bAuth, FAuthCallback OnDone);

	FString BaseUrl = TEXT("http://localhost:8000");
	FString AccessToken;
};
