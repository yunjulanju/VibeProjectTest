# UE5 Auth HTTP Client — 정리 문서

Unreal Engine 5.7 프로젝트 `VibeProjectTest`에 추가한 **인증 서버 HTTP 통신 클라이언트** 정리.
FastAPI 인증 백엔드(`http://localhost:8000`)와 JSON으로 통신하며, 로그인 토큰(JWT)을 보관해 보호된 요청에 자동 첨부한다.

- 엔진: UE 5.7
- 원래 블루프린트 전용 → **C++ 게임 모듈 추가**로 전환
- 통신: UE `HttpModule` + `Json` / `JsonUtilities`
- 사용: C++ 전용, 테스트는 콘솔 명령 + `LogAuth` 로그

---

## 1. 아키텍처

```
UE 콘솔 (Auth.*)  ──►  UAuthSubsystem  ──►  FHttpModule  ──►  FastAPI (localhost:8000)
                          │  AccessToken 보관
                          └─ 보호 요청에 Authorization: Bearer 자동 첨부
```

- **`UAuthSubsystem`** (`UGameInstanceSubsystem`) — 게임 인스턴스 수명 동안 유지되는 단일 객체가 인증 상태를 관리. 어디서든 `GetGameInstance()->GetSubsystem<UAuthSubsystem>()`로 접근.
- **공통 요청 헬퍼** `SendRequest()` — URL/Verb/헤더 구성, 비동기 전송, 응답을 상태코드로 분기해 콜백 + 로그.
- **콘솔 명령** — 각 엔드포인트를 `~` 콘솔에서 바로 호출(테스트용).

---

## 2. 파일 구성

| 파일 | 역할 |
|------|------|
| `Source/VibeProjectTest.Target.cs` | Game 빌드 타깃 (`BuildSettingsVersion.V6`) |
| `Source/VibeProjectTestEditor.Target.cs` | Editor 빌드 타깃 |
| `Source/VibeProjectTest/VibeProjectTest.Build.cs` | 모듈 의존성: `HTTP`, `Json`, `JsonUtilities` |
| `Source/VibeProjectTest/VibeProjectTest.h` / `.cpp` | primary game module 진입점 |
| `Source/VibeProjectTest/Auth/AuthSubsystem.h` / `.cpp` | **핵심** — 인증 서브시스템 |
| `Source/VibeProjectTest/Auth/AuthConsoleCommands.cpp` | `Auth.*` 콘솔 명령 등록 |
| `VibeProjectTest.uproject` | `Modules`에 `VibeProjectTest` 런타임 모듈 등록 |

> 같은 `Auth/` 폴더 내 `.cpp`는 헤더를 `#include "AuthSubsystem.h"`(접두사 없이)로 참조 — 플랫 모듈 구조에서 C1083 방지.

---

## 3. `UAuthSubsystem` API

```cpp
// 상태
void            SetBaseUrl(const FString& InUrl);   // 기본 http://localhost:8000
const FString&  GetBaseUrl() const;
bool            IsLoggedIn() const;                 // AccessToken 보유 여부
void            ClearToken();

// 엔드포인트 (모두 비동기, 완료 시 콜백 호출)
void Health  (FAuthCallback OnDone = {});
void Signup  (Email, Username, Password, FAuthCallback OnDone = {});
void Login   (Email, Password, FAuthCallback OnDone = {});   // 성공 시 access_token 저장
void GetMe   (FAuthCallback OnDone = {});                    // Bearer 자동 첨부
void DeleteMe(FAuthCallback OnDone = {});                    // 성공 시 토큰 클리어
```

콜백 시그니처:
```cpp
using FAuthCallback = TFunction<void(bool bSuccess, int32 StatusCode, const FString& Content)>;
```

### 매핑되는 서버 엔드포인트

| 메서드 | HTTP | 경로 | 인증 | 성공 |
|--------|------|------|------|------|
| `Health` | GET | `/health` | — | 200 |
| `Signup` | POST | `/auth/signup` | — | 201 (중복 409) |
| `Login` | POST | `/auth/login` | — | 200 → `access_token` |
| `GetMe` | GET | `/users/me` | Bearer | 200 (401) |
| `DeleteMe` | DELETE | `/users/me` | Bearer | 204 (401) |

---

## 4. 토큰 흐름

1. `Login` 성공(200) → 응답 JSON에서 `access_token` 파싱 → `AccessToken` 멤버에 저장.
2. 이후 `GetMe` / `DeleteMe` 요청은 `SendRequest(..., bAuth=true)` → `Authorization: Bearer <AccessToken>` 헤더 자동 첨부.
3. 토큰이 없는데 보호 요청을 호출하면 전송하지 않고 경고 로그 후 실패 콜백.
4. `DeleteMe` 성공(204) → `ClearToken()`으로 토큰 제거.

> 토큰은 메모리에만 보관(디스크 저장·자동 재로그인은 범위 밖).

---

## 5. C++에서 호출하는 예시

```cpp
UAuthSubsystem* Auth = GetGameInstance()->GetSubsystem<UAuthSubsystem>();

Auth->Login(TEXT("a@b.com"), TEXT("pw12345678"),
    [Auth](bool bOk, int32 Code, const FString& Body)
    {
        if (bOk)
        {
            Auth->GetMe([](bool bOk2, int32 Code2, const FString& Profile)
            {
                UE_LOG(LogAuth, Log, TEXT("프로필: %s"), *Profile);
            });
        }
    });
```

---

## 6. 콘솔 명령 (테스트)

Play(PIE) 중 `~` 콘솔에서 사용. 결과는 Output Log의 `LogAuth`로 출력.

| 명령 | 동작 |
|------|------|
| `Auth.Health` | `GET /health` |
| `Auth.Signup <email> <username> <password>` | 회원가입 |
| `Auth.Login <email> <password>` | 로그인 + 토큰 저장 |
| `Auth.Me` | 내 정보 조회 (Bearer) |
| `Auth.Delete` | 회원탈퇴 (Bearer) |
| `Auth.SetBaseUrl <url>` | Base URL 변경 |

---

## 7. 빌드 & 실행

### UE 프로젝트 빌드
1. `VibeProjectTest.uproject` 우클릭 → **Generate Visual Studio project files**
2. `VibeProjectTest.sln`을 **Development Editor**로 빌드 (또는 에디터 실행 시 Rebuild 동의)

### 인증 서버 기동 (별도 프로젝트 `C:\Work\VibeCoding`)
```powershell
cd C:\Work\VibeCoding
.\.venv\Scripts\uvicorn.exe app.main:app --reload   # http://localhost:8000/docs
```
> uvicorn은 `.venv`에 설치돼 있음. 서버는 MySQL(`authdb`)에 연결하므로 MySQL 8.4.9가 켜져 있어야 함.

### 검증 시나리오
| 순서 | 명령 | 기대 |
|------|------|------|
| 1 | `Auth.Health` | ✓ 200 |
| 2 | `Auth.Signup a@b.com alice pw12345678` | ✓ 201 (재시도 409) |
| 3 | `Auth.Me` (로그인 전) | "access token 없음" 경고 |
| 4 | `Auth.Login a@b.com pw12345678` | ✓ 200 + 토큰 저장 |
| 5 | `Auth.Me` | ✓ 200 (프로필) |
| 6 | `Auth.Delete` | ✓ 204 + 토큰 클리어 |
| 7 | `Auth.Login ...` (탈퇴 후) | ✗ 401 |

---

## 8. 상태 & 범위

- **완료:** C++ 모듈 추가, 5개 엔드포인트, 토큰 관리, 콘솔 명령, 빌드 통과.
- **남음:** 실서버 대상 콘솔 명령 실동작 검증.
- **범위 밖(YAGNI):** 블루프린트 노출 노드, 토큰 영구 저장/자동 재로그인, refresh token·만료 처리, 테스트 UMG UI.

## 관련 문서
- 설계 스펙: `docs/superpowers/specs/2026-07-15-ue5-auth-http-client-design.md`
- 구현 계획: `docs/superpowers/plans/2026-07-15-ue5-auth-http-client.md`
- 진행 상태: `docs/progress.md` / `docs/progress.html`
- 서버 요약: `C:\Work\VibeCoding\docs\project-summary.html`
