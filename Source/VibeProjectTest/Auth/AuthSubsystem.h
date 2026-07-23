#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/IHttpRequest.h"

struct FActorsInitializedParams;

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
	// --- 서브시스템 수명주기: 월드 BeginPlay에 Auth UI 자동 표시 ---
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

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

	/** 게임 월드 액터 초기화 시점을 잡아 그 월드의 BeginPlay 구독으로 넘긴다. */
	void HandleActorsInitialized(const FActorsInitializedParams& Params);
	/** 해당 월드의 BeginPlay 시점에 Auth 위젯을 생성해 뷰포트에 올린다. */
	void ShowAuthWidget(UWorld* World);

	FDelegateHandle ActorsInitHandle;
	TWeakObjectPtr<class UUserWidget> AuthWidget;

	//Ubuntu IP
	FString BaseUrl = TEXT("http://192.168.0.108:8000");
	FString AccessToken;
};
