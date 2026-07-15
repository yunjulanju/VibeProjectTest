# UE5 Auth HTTP Client — 진행사항

- 최종 업데이트: 2026-07-15
- 프로젝트: `C:\Work\VibeProjectTest` (Unreal Engine 5.7, 블루프린트 전용)
- 통신 대상: FastAPI Auth Server — `http://localhost:8000` (참조: `C:\Work\VibeCoding\docs\project-summary.html`)

## 현재 단계

**C++ 소스 구현 완료(Task 1~6) → 사용자 환경에서 빌드·실서버 검증(Task 7) 대기 중.**
소스 파일은 모두 작성·커밋됨. 컴파일/실행은 아직 하지 않음(이 환경에 UE 빌드 툴 없음).

## 확정된 결정

| 항목 | 결정 |
|------|------|
| 구현 방식 | C++ 게임 모듈 신설 후 UE `HttpModule` 사용 (BP 기본 HTTP 노드 없음) |
| 범위 | 5개 엔드포인트 전부 (signup / login / GET me / DELETE me / health) |
| 사용 방식 | C++에서만 사용, BP 노출 안 함. 콘솔 명령 + 로그로 테스트 |
| Base URL | 기본값 `http://localhost:8000` (변경 가능) |

## 서버 엔드포인트

| 메서드 | 경로 | 인증 | 성공 코드 |
|--------|------|------|-----------|
| POST | `/auth/signup` | — | 201 (중복 409) |
| POST | `/auth/login` | — | 200 (실패 401) → `access_token` |
| GET | `/users/me` | Bearer | 200 (401) |
| DELETE | `/users/me` | Bearer | 204 (401) |
| GET | `/health` | — | 200 |

## 설계 요약

- **신규 C++ 모듈** `Source/VibeProjectTest/` (Build.cs 의존성: `HTTP`, `Json`, `JsonUtilities`), `.uproject`에 `Modules` 등록
- **핵심 클래스** `UAuthSubsystem : UGameInstanceSubsystem`
  - 상태: `BaseUrl`, `AccessToken`
  - 메서드: `Health / Signup / Login / GetMe / DeleteMe`, `IsLoggedIn`
  - 로그인 성공 시 `access_token` 저장 → 보호 요청에 `Authorization: Bearer` 자동 첨부
- **콘솔 명령**으로 테스트: `Auth.Health`, `Auth.Signup <email> <user> <pw>`, `Auth.Login <email> <pw>`, `Auth.Me`, `Auth.Delete`
- **로그 카테고리** `LogAuth`로 결과 출력

## 남은 작업 (다음 단계)

1. [x] 사용자 스펙 검토 승인
2. [x] `writing-plans`로 구현 계획 작성
3. [x] C++ 모듈 스캐폴딩 (Target.cs, Build.cs, 모듈 진입점, `.uproject` 수정)
4. [x] `UAuthSubsystem` 구현 (요청 헬퍼 + 5개 메서드 + 토큰 관리)
5. [x] 콘솔 명령 등록
6. [ ] **(사용자)** Visual Studio + UE5.7에서 빌드 후, 서버 띄우고 콘솔 명령으로 검증

### 사용자 환경에서 할 일 (빌드 & 검증)

1. `VibeProjectTest.uproject` 우클릭 → **Generate Visual Studio project files**
2. `VibeProjectTest.sln`을 **Development Editor**로 빌드 (또는 에디터 실행 시 Rebuild 프롬프트 동의)
3. FastAPI 서버 기동: `uvicorn app.main:app --reload` (`http://localhost:8000/docs` 확인)
4. 에디터에서 **Play(PIE)** 시작 → 콘솔(`~`) 열고 Output Log의 `LogAuth` 관찰
5. 순서대로 실행:
   - `Auth.Health` → 200
   - `Auth.Signup a@b.com alice pw12345678` → 201 (재시도 시 409)
   - `Auth.Me` (로그인 전) → "access token 없음" 경고
   - `Auth.Login a@b.com pw12345678` → 200 + 토큰 저장
   - `Auth.Me` → 200 (프로필)
   - `Auth.Delete` → 204 + 토큰 클리어
   - `Auth.Login a@b.com pw12345678` (탈퇴 후) → 401

## 구현된 파일

- `Source/VibeProjectTest.Target.cs`, `Source/VibeProjectTestEditor.Target.cs`
- `Source/VibeProjectTest/VibeProjectTest.Build.cs` / `.h` / `.cpp`
- `Source/VibeProjectTest/Auth/AuthSubsystem.h` / `.cpp` — 핵심 인증 서브시스템
- `Source/VibeProjectTest/Auth/AuthConsoleCommands.cpp` — `Auth.*` 콘솔 명령
- `VibeProjectTest.uproject` — `Modules` 등록

## 참고

- 상세 설계: `docs/superpowers/specs/2026-07-15-ue5-auth-http-client-design.md`
- 빌드/실행은 사용자 환경(Visual Studio C++ 워크로드 + UE5.7)에서 진행 필요
- 이 프로젝트는 아직 git 저장소가 아님 (커밋 원하면 `git init` 필요)
