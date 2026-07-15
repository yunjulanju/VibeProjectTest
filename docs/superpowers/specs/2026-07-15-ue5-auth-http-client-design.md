# UE5 Auth HTTP Client — 설계 문서

- 날짜: 2026-07-15
- 대상 프로젝트: `C:\Work\VibeProjectTest` (Unreal Engine 5.7, 블루프린트 전용)
- 통신 대상 서버: FastAPI Auth Server (`project-summary.html` 기준), 기본 `http://localhost:8000`

## 1. 목표

UE5 클라이언트에서 FastAPI 인증 백엔드와 HTTP(JSON)로 통신한다. 회원가입 · 로그인 · 내 정보 조회 · 회원탈퇴 · 헬스체크를 지원하고, 로그인으로 발급받은 JWT access token을 저장했다가 보호된 요청 헤더(`Authorization: Bearer <token>`)에 자동 첨부한다.

## 2. 서버 인터페이스 (참조)

| 메서드 | 경로 | 인증 | 성공 | 설명 |
|--------|------|------|------|------|
| POST | `/auth/signup` | — | 201 (중복 409) | email + username + password(8자+) |
| POST | `/auth/login` | — | 200 (실패 401) | email + password → `access_token` |
| GET | `/users/me` | Bearer | 200 (401) | 내 프로필 |
| DELETE | `/users/me` | Bearer | 204 (401) | soft delete |
| GET | `/health` | — | 200 | 헬스체크 |

## 3. 결정 사항

- **구현 방식:** C++ 모듈 추가. UE `HttpModule` 사용. (블루프린트 기본 HTTP 노드가 없기 때문)
- **범위:** 위 5개 엔드포인트 전부.
- **사용 방식:** C++에서만 사용. 블루프린트 노출은 하지 않는다. 테스트는 콘솔 명령 + 로그로 확인.
- **Base URL 기본값:** `http://localhost:8000` (멤버 변수로 변경 가능).

## 4. 아키텍처

### 4.1 신규 C++ 게임 모듈

현재 프로젝트에 C++ 모듈이 없으므로 1차 게임 모듈을 신설한다.

```
Source/
  VibeProjectTest.Target.cs
  VibeProjectTestEditor.Target.cs
  VibeProjectTest/
    VibeProjectTest.Build.cs        # PublicDependencyModuleNames: Core, CoreUObject, Engine, HTTP, Json, JsonUtilities
    VibeProjectTest.h / .cpp        # IMPLEMENT_PRIMARY_GAME_MODULE
    Auth/
      AuthSubsystem.h / .cpp
      AuthConsoleCommands.cpp       # (선택) 콘솔 명령 등록
```

`.uproject`에 `Modules` 배열을 추가한다:
```json
"Modules": [
  { "Name": "VibeProjectTest", "Type": "Runtime", "LoadingPhase": "Default" }
]
```

> 빌드는 Visual Studio(C++ 워크로드) + UE5.7 환경에서 수행. 소스는 이 작업에서 생성하지만 최종 컴파일/실행은 사용자 환경에서 진행한다.

### 4.2 핵심 클래스: `UAuthSubsystem`

`UGameInstanceSubsystem`을 상속. 게임 인스턴스 수명 동안 유지되며 어디서든
`GetGameInstance()->GetSubsystem<UAuthSubsystem>()`로 접근 가능. 인증 상태를 한 곳에서 관리한다.

상태:
- `FString BaseUrl = TEXT("http://localhost:8000");`
- `FString AccessToken;` (로그인 성공 시 저장, 탈퇴/로그아웃 시 클리어)

공개 메서드 (모두 비동기, 완료 시 `TFunction<void(bool bSuccess, int32 StatusCode, const FString& Body)>` 콜백 지원, 콜백은 선택):
- `void Health(FAuthResponseDelegate OnDone = {});`
- `void Signup(const FString& Email, const FString& Username, const FString& Password, ...);`
- `void Login(const FString& Email, const FString& Password, ...);` — 200이면 응답 JSON에서 `access_token`을 파싱해 `AccessToken`에 저장.
- `void GetMe(...);` — Bearer 자동 첨부.
- `void DeleteMe(...);` — Bearer 자동 첨부, 204면 `AccessToken` 클리어.
- `bool IsLoggedIn() const { return !AccessToken.IsEmpty(); }`

### 4.3 요청 헬퍼 (내부)

공통 private 헬퍼로 중복 제거:
```
TSharedRef<IHttpRequest> MakeRequest(const FString& Path, const FString& Verb, bool bAuth);
```
- URL = `BaseUrl + Path`, Verb 설정
- `Content-Type: application/json` 헤더
- `bAuth == true`이면 `Authorization: Bearer <AccessToken>` 첨부. 토큰이 비어 있으면 요청을 보내지 않고 경고 로그 후 콜백에 실패 통지.
- 요청 본문 JSON은 `TJsonWriterFactory`로 직렬화.
- 응답은 `OnProcessRequestComplete`에서 `bWasSuccessful` + `GetResponseCode()`로 분기하고, 필요 시 `TJsonReaderFactory`로 파싱.

### 4.4 테스트 트리거 — 콘솔 명령

서버가 떠 있는 상태에서 UE 콘솔(`~`)로 각 엔드포인트를 호출할 수 있게 `FAutoConsoleCommandWithWorldAndArgs`(또는 `FAutoConsoleCommand`)를 등록한다. 명령은 현재 월드의 GameInstance에서 서브시스템을 찾아 호출한다.

- `Auth.Health`
- `Auth.Signup <email> <username> <password>`
- `Auth.Login <email> <password>`
- `Auth.Me`
- `Auth.Delete`
- `Auth.SetBaseUrl <url>` (선택 — 기본값 변경용)

결과는 전용 로그 카테고리 `LogAuth`로 출력한다.

## 5. 에러 처리

- 네트워크 실패(`bWasSuccessful == false`) → 실패 콜백 + `LogAuth` Error.
- 상태코드 분기: 2xx 성공, 401 인증 실패, 409 중복 등 코드와 응답 본문을 로그로 남김.
- JSON 파싱 실패 방어(로그인 응답에 `access_token` 없으면 실패 처리).
- 보호 요청인데 토큰이 없으면 즉시 경고 로그 후 요청하지 않음.

## 6. 검증 방법

C++ + UE HttpModule 특성상 순수 유닛 테스트는 어렵다. 실서버(문서의 FastAPI)를 `http://localhost:8000`에 띄운 상태에서 콘솔 명령으로 수동 검증한다:

1. `Auth.Health` → 200 로그
2. `Auth.Signup a@b.com alice pw12345678` → 201, 재시도 시 409
3. `Auth.Login a@b.com pw12345678` → 200 + 토큰 저장 로그
4. `Auth.Me` → 200 프로필, (로그인 전 호출 시 토큰 없음 경고)
5. `Auth.Delete` → 204 + 토큰 클리어
6. 탈퇴 후 `Auth.Login` → 401

## 7. 범위 밖 (YAGNI)

- 블루프린트 노출 노드
- 토큰 영구 저장(디스크/SaveGame), 자동 재로그인
- refresh token, 토큰 만료 처리
- UMG 테스트 UI
