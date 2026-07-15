# UE5 Auth HTTP Client Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 블루프린트 전용 UE5.7 프로젝트에 C++ 게임 모듈을 추가하고, FastAPI 인증 서버(`http://localhost:8000`)와 HTTP·JSON으로 통신하는 `UAuthSubsystem`을 구현한다.

**Architecture:** UE `HttpModule`을 쓰는 C++ 게임 모듈을 신설한다. `UGameInstanceSubsystem` 하나가 인증 상태(access token)를 보관하고, 공통 요청 헬퍼로 5개 엔드포인트를 비동기 호출한다. 각 호출은 `TFunction` 콜백 + `LogAuth` 로그로 결과를 알린다. 테스트/실행 트리거는 콘솔 명령으로 노출한다.

**Tech Stack:** Unreal Engine 5.7, C++, 모듈 `HTTP` / `Json` / `JsonUtilities`.

## Global Constraints

- 엔진 버전: **UE 5.7** (`.uproject` EngineAssociation `5.7`).
- 모듈 이름: **`VibeProjectTest`** (프로젝트명과 동일, primary game module).
- 서버 Base URL 기본값: **`http://localhost:8000`** (런타임 변경 가능).
- 보호 엔드포인트(`GET/DELETE /users/me`)는 **`Authorization: Bearer <AccessToken>`** 자동 첨부.
- 콜백 시그니처는 전 태스크 공통: **`TFunction<void(bool bSuccess, int32 StatusCode, const FString& Content)>`** (typedef `FAuthCallback`).
- 로그 카테고리: **`LogAuth`**.
- 빌드/실행 환경: Visual Studio(C++ 워크로드) + UE5.7. 이 계획의 각 "빌드 검증"은 사용자 환경에서 수행한다.
- 이 프로젝트는 git 저장소가 아니므로 Task 1에서 `git init` 후, 각 태스크 끝에서 커밋한다.

## File Structure

- `Source/VibeProjectTest.Target.cs` — Game 타깃 정의
- `Source/VibeProjectTestEditor.Target.cs` — Editor 타깃 정의
- `Source/VibeProjectTest/VibeProjectTest.Build.cs` — 모듈 의존성
- `Source/VibeProjectTest/VibeProjectTest.h` / `.cpp` — 모듈 진입점
- `Source/VibeProjectTest/Auth/AuthSubsystem.h` / `.cpp` — 핵심 인증 서브시스템
- `Source/VibeProjectTest/Auth/AuthConsoleCommands.cpp` — 테스트용 콘솔 명령 등록
- `VibeProjectTest.uproject` (수정) — `Modules` 배열 추가

---

### Task 1: C++ 모듈 스캐폴딩

블루프린트 전용 프로젝트를 C++ 하이브리드로 전환한다. 빈 primary game module이 붙고 컴파일되는 것이 이 태스크의 산출물이다.

**Files:**
- Create: `Source/VibeProjectTest.Target.cs`
- Create: `Source/VibeProjectTestEditor.Target.cs`
- Create: `Source/VibeProjectTest/VibeProjectTest.Build.cs`
- Create: `Source/VibeProjectTest/VibeProjectTest.h`
- Create: `Source/VibeProjectTest/VibeProjectTest.cpp`
- Modify: `VibeProjectTest.uproject`

**Interfaces:**
- Consumes: (없음)
- Produces: `VibeProjectTest` 런타임 모듈. Build.cs에 `HTTP`, `Json`, `JsonUtilities` 의존성이 준비되어 이후 태스크가 사용한다.

- [ ] **Step 1: git 저장소 초기화**

```bash
cd /c/Work/VibeProjectTest
git init
printf '%s\n' 'Binaries/' 'Build/' 'DerivedDataCache/' 'Intermediate/' 'Saved/' '.vs/' '*.sln' > .gitignore
git add .gitignore
git commit -m "chore: git init with UE gitignore"
```

- [ ] **Step 2: Game 타깃 파일 작성**

`Source/VibeProjectTest.Target.cs`:
```csharp
using UnrealBuildTool;
using System.Collections.Generic;

public class VibeProjectTestTarget : TargetRules
{
	public VibeProjectTestTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("VibeProjectTest");
	}
}
```

- [ ] **Step 3: Editor 타깃 파일 작성**

`Source/VibeProjectTestEditor.Target.cs`:
```csharp
using UnrealBuildTool;
using System.Collections.Generic;

public class VibeProjectTestEditorTarget : TargetRules
{
	public VibeProjectTestEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("VibeProjectTest");
	}
}
```

- [ ] **Step 4: Build.cs 작성 (HTTP/Json 의존성 포함)**

`Source/VibeProjectTest/VibeProjectTest.Build.cs`:
```csharp
using UnrealBuildTool;

public class VibeProjectTest : ModuleRules
{
	public VibeProjectTest(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"HTTP", "Json", "JsonUtilities"
		});
	}
}
```

- [ ] **Step 5: 모듈 진입점 작성**

`Source/VibeProjectTest/VibeProjectTest.h`:
```cpp
#pragma once

#include "CoreMinimal.h"
```

`Source/VibeProjectTest/VibeProjectTest.cpp`:
```cpp
#include "VibeProjectTest.h"
#include "Modules/ModuleManager.h"

IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl, VibeProjectTest, "VibeProjectTest");
```

- [ ] **Step 6: `.uproject`에 Modules 등록**

`VibeProjectTest.uproject`를 아래처럼 수정 (기존 `Plugins` 배열 유지, `Modules` 추가):
```json
{
	"FileVersion": 3,
	"EngineAssociation": "5.7",
	"Category": "",
	"Description": "",
	"Modules": [
		{
			"Name": "VibeProjectTest",
			"Type": "Runtime",
			"LoadingPhase": "Default"
		}
	],
	"Plugins": [
		{
			"Name": "ModelingToolsEditorMode",
			"Enabled": true,
			"TargetAllowList": [
				"Editor"
			]
		}
	]
}
```

- [ ] **Step 7: 프로젝트 파일 생성 + 빌드 검증**

사용자 환경에서 실행:
- `.uproject` 우클릭 → "Generate Visual Studio project files"
- 생성된 `VibeProjectTest.sln`을 Development Editor 구성으로 빌드 (또는 에디터 실행 시 "Rebuild" 프롬프트에 동의)

Expected: 컴파일 성공, 에디터가 정상 기동. 실패 시 다음 태스크로 넘어가지 않는다.

- [ ] **Step 8: Commit**

```bash
git add Source VibeProjectTest.uproject
git commit -m "feat: scaffold VibeProjectTest C++ game module with HTTP/Json deps"
```

---

### Task 2: AuthSubsystem 골격 + 로그 + 요청 헬퍼

인증 서브시스템의 뼈대와 공통 요청 헬퍼를 만든다. 이 태스크 이후 빈 엔드포인트 메서드들이 존재하고 컴파일된다.

**Files:**
- Create: `Source/VibeProjectTest/Auth/AuthSubsystem.h`
- Create: `Source/VibeProjectTest/Auth/AuthSubsystem.cpp`

**Interfaces:**
- Consumes: `VibeProjectTest` 모듈, `HTTP`/`Json` 의존성 (Task 1).
- Produces:
  - `using FAuthCallback = TFunction<void(bool bSuccess, int32 StatusCode, const FString& Content)>;`
  - `UAuthSubsystem : public UGameInstanceSubsystem`
  - 멤버: `FString BaseUrl;`, `FString AccessToken;`
  - `void SetBaseUrl(const FString& InUrl);`
  - `bool IsLoggedIn() const;`
  - 내부 헬퍼: `void SendRequest(const FString& Path, const FString& Verb, const FString& Body, bool bAuth, FAuthCallback OnDone);`
  - Task 3~5가 위 `SendRequest`를 호출한다.

- [ ] **Step 1: 헤더 작성**

`Source/VibeProjectTest/Auth/AuthSubsystem.h`:
```cpp
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
```

- [ ] **Step 2: cpp 작성 (로그 카테고리 + SendRequest 헬퍼)**

`Source/VibeProjectTest/Auth/AuthSubsystem.cpp`:
```cpp
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
```

- [ ] **Step 3: 빌드 검증**

사용자 환경에서 Development Editor 빌드.
Expected: 컴파일 성공 (미사용 파라미터 경고는 스텁이라 무시 가능).

- [ ] **Step 4: Commit**

```bash
git add Source/VibeProjectTest/Auth/AuthSubsystem.h Source/VibeProjectTest/Auth/AuthSubsystem.cpp
git commit -m "feat: add AuthSubsystem skeleton with SendRequest helper and LogAuth"
```

---

### Task 3: Health + Signup 구현

인증이 필요 없는 두 엔드포인트를 채운다. JSON 직렬화 패턴을 여기서 확립한다.

**Files:**
- Modify: `Source/VibeProjectTest/Auth/AuthSubsystem.cpp` (Health, Signup 본문)

**Interfaces:**
- Consumes: `SendRequest(Path, Verb, Body, bAuth, OnDone)` (Task 2).
- Produces: 동작하는 `Health()`, `Signup(Email, Username, Password, OnDone)`. Signup 본문 JSON 키: `email`, `username`, `password`.

- [ ] **Step 1: JSON 직렬화 include 추가**

`AuthSubsystem.cpp` 상단 include에 추가:
```cpp
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
```

- [ ] **Step 2: Health 구현**

`AuthSubsystem.cpp`의 `Health` 스텁을 교체:
```cpp
void UAuthSubsystem::Health(FAuthCallback OnDone)
{
	SendRequest(TEXT("/health"), TEXT("GET"), FString(), /*bAuth=*/false, MoveTemp(OnDone));
}
```

- [ ] **Step 3: Signup 구현**

`AuthSubsystem.cpp`의 `Signup` 스텁을 교체:
```cpp
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
```

- [ ] **Step 4: 빌드 검증**

Development Editor 빌드.
Expected: 컴파일 성공.

- [ ] **Step 5: Commit**

```bash
git add Source/VibeProjectTest/Auth/AuthSubsystem.cpp
git commit -m "feat: implement Health and Signup endpoints"
```

---

### Task 4: Login (토큰 저장) + GetMe (Bearer)

로그인 성공 시 응답 JSON에서 `access_token`을 파싱해 저장하고, 보호 엔드포인트 GetMe가 그 토큰을 쓰도록 한다.

**Files:**
- Modify: `Source/VibeProjectTest/Auth/AuthSubsystem.cpp` (Login, GetMe 본문)

**Interfaces:**
- Consumes: `SendRequest` (Task 2), JSON include (Task 3), 멤버 `AccessToken`.
- Produces: `Login(Email, Password, OnDone)` — 200이면 `AccessToken` 저장. `GetMe(OnDone)` — `bAuth=true`.

- [ ] **Step 1: Login 구현 (응답에서 access_token 파싱)**

`AuthSubsystem.cpp`의 `Login` 스텁을 교체:
```cpp
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
```

- [ ] **Step 2: GetMe 구현**

`AuthSubsystem.cpp`의 `GetMe` 스텁을 교체:
```cpp
void UAuthSubsystem::GetMe(FAuthCallback OnDone)
{
	SendRequest(TEXT("/users/me"), TEXT("GET"), FString(), /*bAuth=*/true, MoveTemp(OnDone));
}
```

- [ ] **Step 3: 빌드 검증**

Development Editor 빌드.
Expected: 컴파일 성공.

- [ ] **Step 4: Commit**

```bash
git add Source/VibeProjectTest/Auth/AuthSubsystem.cpp
git commit -m "feat: implement Login with token storage and GetMe with Bearer"
```

---

### Task 5: DeleteMe (탈퇴 후 토큰 클리어)

**Files:**
- Modify: `Source/VibeProjectTest/Auth/AuthSubsystem.cpp` (DeleteMe 본문)

**Interfaces:**
- Consumes: `SendRequest` (Task 2), `ClearToken()` (Task 2).
- Produces: `DeleteMe(OnDone)` — `bAuth=true`, 성공(204) 시 토큰 클리어.

- [ ] **Step 1: DeleteMe 구현**

`AuthSubsystem.cpp`의 `DeleteMe` 스텁을 교체:
```cpp
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
```

- [ ] **Step 2: 빌드 검증**

Development Editor 빌드.
Expected: 컴파일 성공.

- [ ] **Step 3: Commit**

```bash
git add Source/VibeProjectTest/Auth/AuthSubsystem.cpp
git commit -m "feat: implement DeleteMe with token clear on success"
```

---

### Task 6: 테스트용 콘솔 명령 등록

UE 콘솔(`~`)에서 각 엔드포인트를 호출할 수 있게 한다. 현재 월드의 GameInstance에서 서브시스템을 찾아 호출한다.

**Files:**
- Create: `Source/VibeProjectTest/Auth/AuthConsoleCommands.cpp`

**Interfaces:**
- Consumes: `UAuthSubsystem` 전체 공개 API (Task 2~5).
- Produces: 콘솔 명령 `Auth.Health`, `Auth.Signup`, `Auth.Login`, `Auth.Me`, `Auth.Delete`, `Auth.SetBaseUrl`.

- [ ] **Step 1: 콘솔 명령 파일 작성**

`Source/VibeProjectTest/Auth/AuthConsoleCommands.cpp`:
```cpp
#include "Auth/AuthSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"

namespace
{
	// 현재 활성 월드의 GameInstance에서 AuthSubsystem을 찾는다.
	UAuthSubsystem* FindAuth(UWorld* World)
	{
		if (!World)
		{
			UE_LOG(LogAuth, Error, TEXT("월드가 없어 AuthSubsystem을 찾을 수 없음"));
			return nullptr;
		}
		UGameInstance* GI = World->GetGameInstance();
		UAuthSubsystem* Auth = GI ? GI->GetSubsystem<UAuthSubsystem>() : nullptr;
		if (!Auth)
		{
			UE_LOG(LogAuth, Error, TEXT("AuthSubsystem을 찾을 수 없음 (PIE/게임 실행 중인지 확인)"));
		}
		return Auth;
	}

	FAutoConsoleCommandWithWorldAndArgs GAuthHealth(
		TEXT("Auth.Health"), TEXT("GET /health"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
			[](const TArray<FString>& /*Args*/, UWorld* World)
			{
				if (UAuthSubsystem* Auth = FindAuth(World)) { Auth->Health(); }
			}));

	FAutoConsoleCommandWithWorldAndArgs GAuthSignup(
		TEXT("Auth.Signup"), TEXT("POST /auth/signup — 사용법: Auth.Signup <email> <username> <password>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
			[](const TArray<FString>& Args, UWorld* World)
			{
				if (Args.Num() < 3)
				{
					UE_LOG(LogAuth, Warning, TEXT("사용법: Auth.Signup <email> <username> <password>"));
					return;
				}
				if (UAuthSubsystem* Auth = FindAuth(World)) { Auth->Signup(Args[0], Args[1], Args[2]); }
			}));

	FAutoConsoleCommandWithWorldAndArgs GAuthLogin(
		TEXT("Auth.Login"), TEXT("POST /auth/login — 사용법: Auth.Login <email> <password>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
			[](const TArray<FString>& Args, UWorld* World)
			{
				if (Args.Num() < 2)
				{
					UE_LOG(LogAuth, Warning, TEXT("사용법: Auth.Login <email> <password>"));
					return;
				}
				if (UAuthSubsystem* Auth = FindAuth(World)) { Auth->Login(Args[0], Args[1]); }
			}));

	FAutoConsoleCommandWithWorldAndArgs GAuthMe(
		TEXT("Auth.Me"), TEXT("GET /users/me (Bearer)"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
			[](const TArray<FString>& /*Args*/, UWorld* World)
			{
				if (UAuthSubsystem* Auth = FindAuth(World)) { Auth->GetMe(); }
			}));

	FAutoConsoleCommandWithWorldAndArgs GAuthDelete(
		TEXT("Auth.Delete"), TEXT("DELETE /users/me (Bearer)"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
			[](const TArray<FString>& /*Args*/, UWorld* World)
			{
				if (UAuthSubsystem* Auth = FindAuth(World)) { Auth->DeleteMe(); }
			}));

	FAutoConsoleCommandWithWorldAndArgs GAuthSetBaseUrl(
		TEXT("Auth.SetBaseUrl"), TEXT("Base URL 변경 — 사용법: Auth.SetBaseUrl <url>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
			[](const TArray<FString>& Args, UWorld* World)
			{
				if (Args.Num() < 1)
				{
					UE_LOG(LogAuth, Warning, TEXT("사용법: Auth.SetBaseUrl <url>"));
					return;
				}
				if (UAuthSubsystem* Auth = FindAuth(World))
				{
					Auth->SetBaseUrl(Args[0]);
					UE_LOG(LogAuth, Log, TEXT("Base URL = %s"), *Auth->GetBaseUrl());
				}
			}));
}
```

- [ ] **Step 2: 빌드 검증**

Development Editor 빌드.
Expected: 컴파일 성공, 에디터 기동.

- [ ] **Step 3: Commit**

```bash
git add Source/VibeProjectTest/Auth/AuthConsoleCommands.cpp
git commit -m "feat: add Auth.* console commands for endpoint testing"
```

---

### Task 7: 실서버 대상 런타임 검증

서버를 띄운 상태에서 콘솔 명령으로 전체 플로우를 수동 검증한다. (자동 유닛 테스트는 UE HTTP 특성상 이 계획에서 제외.)

**Files:** (코드 변경 없음 — 검증만)

**Interfaces:**
- Consumes: Task 1~6 전체.

- [ ] **Step 1: 서버 기동**

`project-summary.html` 안내대로 FastAPI 서버를 `http://localhost:8000`에 띄운다:
```bash
uvicorn app.main:app --reload
```
`http://localhost:8000/docs`가 열리는지 확인.

- [ ] **Step 2: UE 에디터에서 Play(PIE) 시작 후 콘솔(`~`) 열기**

Output Log 창을 열어 `LogAuth` 카테고리 출력을 관찰한다.

- [ ] **Step 3: 순서대로 명령 실행 및 기대 결과 확인**

| 명령 | 기대 로그 |
|------|-----------|
| `Auth.Health` | `✓ ... 200` |
| `Auth.Signup a@b.com alice pw12345678` | `✓ ... 201` |
| `Auth.Signup a@b.com alice pw12345678` (재시도) | `✗ ... 409` |
| `Auth.Me` (로그인 전) | `access token 없음` 경고 |
| `Auth.Login a@b.com pw12345678` | `✓ ... 200` + `access token 저장됨` |
| `Auth.Me` | `✓ ... 200` (프로필 JSON) |
| `Auth.Delete` | `✓ ... 204` + `토큰 클리어됨` |
| `Auth.Login a@b.com pw12345678` (탈퇴 후) | `✗ ... 401` |

- [ ] **Step 4: 결과 기록 및 진행사항 갱신**

`docs/progress.md`와 `docs/progress.html`의 "남은 작업" 체크리스트를 완료로 갱신하고 커밋:
```bash
git add docs/progress.md docs/progress.html
git commit -m "docs: mark auth http client verified against live server"
```

---

## 검증 노트

UE `HttpModule`은 엔진 런타임(PIE/게임)이 필요해 순수 유닛 테스트(pytest 류)를 붙이기 어렵다. 이 계획의 검증 전략은 (1) 각 태스크의 **컴파일 성공** + (2) Task 7의 **실서버 대상 콘솔 명령 수동 검증**이다. 향후 필요 시 UE Automation Spec(`FAutomationTestBase`)으로 JSON 직렬화 부분만 분리해 테스트할 수 있으나 현 범위 밖(YAGNI).
