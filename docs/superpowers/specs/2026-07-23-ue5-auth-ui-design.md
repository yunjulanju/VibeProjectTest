# 설계: UE5 인증 UI (C++ 로직 + WBP 배치, Play 시 자동 표시)

## 1. 배경 / 목적

기존에는 `UAuthSubsystem`(FastAPI 인증 서버 연동)을 `~` 콘솔의 `Auth.*` 명령으로만
호출했다. 이를 **화면 UI**로 조작할 수 있게 한다. 회원가입·로그인·내 정보(Me)·
탈퇴(Delete)·헬스체크 5개 기능을 버튼/입력란으로 노출하고, 로그인 상태에 따라
버튼을 활성/비활성한다.

- 통신 대상/상태: 기존 `UAuthSubsystem` 그대로 사용. 서버 기본 `http://localhost:8000`.
- UI 범위: 위 5개 기능. **Base URL 입력란은 이번 범위 밖**(기본값 사용).

## 2. 접근 방식 (확정)

- **위젯 로직은 전부 C++** (`UAuthWidgetBase : UUserWidget`). WBP는 비주얼 껍데기만 담당.
  → subsystem을 Blueprint로 노출할 필요 없음. C++에서 기존 `FAuthCallback`(TFunction)
    API를 직접 호출해 UI를 갱신한다.
- **표시 시점:** Play(PIE) 시작 시 자동 표시. 표시 훅은 **`UAuthSubsystem`에 몇 줄 추가**
  (월드 BeginPlay 구독 → 설정된 WBP 생성·뷰포트 추가). 별도 매니저 클래스 신설 안 함(YAGNI).
- **표시할 WBP 클래스 지정:** `UDeveloperSettings`로 Project Settings에 노출.
  경로/이름 하드코딩 대신 드롭다운으로 선택 → 오타/경로 실수 방지.

## 3. 구성요소

### 신규 C++

| 파일 | 클래스 | 역할 |
|---|---|---|
| `Auth/AuthWidgetBase.h/.cpp` | `UAuthWidgetBase : UUserWidget` | 버튼 핸들러, 결과 표시, 로그인 상태 기반 버튼 활성화 |
| `Auth/AuthUISettings.h/.cpp` | `UAuthUISettings : UDeveloperSettings` | `TSoftClassPtr<UUserWidget> AuthWidgetClass` 하나 (Project Settings) |

### 기존 수정

| 파일 | 변경 |
|---|---|
| `Auth/AuthSubsystem.h/.cpp` | 월드 BeginPlay 구독 + 자동표시 헬퍼(몇 줄). `Initialize`에서 델리게이트 바인딩, `Deinitialize`에서 해제 |
| `VibeProjectTest.Build.cs` | `UMG`, `Slate`, `SlateCore`, `DeveloperSettings` 의존성 추가 |

## 4. `UAuthWidgetBase` — 요구 위젯 (BindWidget)

WBP에서 아래 **정확한 이름**으로 배치해야 C++가 바인딩한다. 레이아웃은 자유.

- `UEditableTextBox` — **EmailBox**, **UsernameBox**, **PasswordBox**(`IsPassword=true` 권장)
- `UButton` — **SignupButton**, **LoginButton**, **MeButton**, **DeleteButton**, **HealthButton**
- `UTextBlock` — **StatusText**(마지막 결과/에러), **LoginStateText**(선택, 현재 로그인 이메일)

`meta=(BindWidget)`. `LoginStateText`는 `meta=(BindWidgetOptional)`로 두어 없어도 컴파일/실행 가능.

## 5. 동작 흐름

1. `NativeConstruct()`에서 각 `UButton::OnClicked`를 대응 `UFUNCTION` 핸들러에 `AddDynamic`.
   초기 버튼 상태 갱신.
2. 핸들러: 텍스트박스 값 읽기 → `GetGameInstance()->GetSubsystem<UAuthSubsystem>()`로
   subsystem 얻어 `Signup/Login/GetMe/DeleteMe/Health(...)` 호출. 콜백 람다에서:
   - `StatusText`를 `✓/✗ + HTTP 상태코드 + 요약`으로 갱신
   - `RefreshButtons()` 호출 → `IsLoggedIn()` 기준 **Me·Delete 활성/비활성**, 로그인 전 비활성
   - `LoginStateText`(있으면) 갱신
3. 입력 검증은 최소한만: 로그인/회원가입 시 빈 값이면 `StatusText`에 안내 후 요청 생략.
   (이메일 형식 등 상세 검증은 서버가 422로 응답 → 그대로 표시.)

## 6. 자동 표시 (subsystem)

- `UAuthSubsystem::Initialize`에서 `FWorldDelegates::OnWorldBeginPlay`(또는 동등 훅) 구독.
- 콜백: `UAuthUISettings`에서 `AuthWidgetClass` 로드 → 유효하면 `CreateWidget` →
  `AddToViewport` → 입력모드 UI + 마우스 커서 표시. 클래스 미지정 시 경고 로그만.
- `Deinitialize`에서 구독 해제. 위젯 중복 생성 방지(약한 참조 보관 또는 1회 플래그).

## 7. 사용자 에디터 작업 (빌드 후)

1. WBP 생성 → 부모 클래스를 `AuthWidgetBase`로 **reparent**
2. 4절 이름들로 버튼·텍스트박스 배치
3. **Project Settings → (Game) Auth UI → Auth Widget Class** = 방금 만든 WBP
4. Play → UI 자동 등장

## 8. 검증

- **컴파일 성공** (신규/수정 C++).
- **PIE 수동 검증** (서버 기동 상태):
  - Play 시 UI 자동 표시, 마우스로 클릭 가능
  - 회원가입 → 로그인 성공 시 **Me·Delete 활성화**
  - Me 조회 → Delete → 로그인 상태 해제되어 **Me·Delete 재비활성화**
  - 각 단계 `StatusText` 표시가 `LogAuth` 로그와 일치
  - 서버 미기동 시 `StatusText`에 네트워크 실패 표시
- 자동 테스트는 UE HTTP/UMG 특성상 이 범위에서 제외(기존 스펙과 동일 전략).

## 9. 범위 밖 (YAGNI)

- Base URL 입력란, 토큰 표시/복사, 폼 애니메이션, 다국어, 저장/자동로그인.
- 별도 UI 매니저/서브시스템 클래스.
