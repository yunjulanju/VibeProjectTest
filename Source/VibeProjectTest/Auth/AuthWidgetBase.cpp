#include "AuthWidgetBase.h"
#include "AuthSubsystem.h"
#include "Components/Button.h"
#include "Components/EditableText.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

void UAuthWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();

	if (SignupButton) { SignupButton->OnClicked.AddDynamic(this, &UAuthWidgetBase::OnSignupClicked); }
	if (LoginButton)  { LoginButton->OnClicked.AddDynamic(this, &UAuthWidgetBase::OnLoginClicked); }
	if (MeButton)     { MeButton->OnClicked.AddDynamic(this, &UAuthWidgetBase::OnMeClicked); }
	if (DeleteButton) { DeleteButton->OnClicked.AddDynamic(this, &UAuthWidgetBase::OnDeleteClicked); }
	if (HealthButton) { HealthButton->OnClicked.AddDynamic(this, &UAuthWidgetBase::OnHealthClicked); }

	RefreshButtons();
}

UAuthSubsystem* UAuthWidgetBase::GetAuth() const
{
	if (const UWorld* World = GetWorld())
	{
		if (UGameInstance* GI = World->GetGameInstance())
		{
			return GI->GetSubsystem<UAuthSubsystem>();
		}
	}
	return nullptr;
}

void UAuthWidgetBase::RefreshButtons()
{
	const UAuthSubsystem* Auth = GetAuth();
	const bool bLoggedIn = Auth && Auth->IsLoggedIn();

	if (MeButton)     { MeButton->SetIsEnabled(bLoggedIn); }
	if (DeleteButton) { DeleteButton->SetIsEnabled(bLoggedIn); }
	if (LoginStateText)
	{
		LoginStateText->SetText(FText::FromString(bLoggedIn ? TEXT("로그인됨") : TEXT("로그아웃 상태")));
	}
}

void UAuthWidgetBase::SetStatus(bool bSuccess, int32 StatusCode, const FString& Content)
{
	if (StatusText)
	{
		const TCHAR* Mark = bSuccess ? TEXT("O") : TEXT("X");
		StatusText->SetText(FText::FromString(FString::Printf(TEXT("%s [%d] %s"), Mark, StatusCode, *Content)));
	}
	RefreshButtons();
}

void UAuthWidgetBase::OnSignupClicked()
{
	UAuthSubsystem* Auth = GetAuth();
	if (!Auth) { return; }

	const FString Email    = EmailBox    ? EmailBox->GetText().ToString()    : FString();
	const FString Username = UsernameBox ? UsernameBox->GetText().ToString() : FString();
	const FString Password = PasswordBox ? PasswordBox->GetText().ToString() : FString();
	if (Email.IsEmpty() || Username.IsEmpty() || Password.IsEmpty())
	{
		if (StatusText) { StatusText->SetText(FText::FromString(TEXT("email/username/password를 모두 입력하세요"))); }
		return;
	}

	TWeakObjectPtr<UAuthWidgetBase> WeakThis(this);
	Auth->Signup(Email, Username, Password,
		[WeakThis](bool bOk, int32 Code, const FString& Content)
		{
			if (UAuthWidgetBase* Self = WeakThis.Get()) { Self->SetStatus(bOk, Code, Content); }
		});
}

void UAuthWidgetBase::OnLoginClicked()
{
	UAuthSubsystem* Auth = GetAuth();
	if (!Auth) { return; }

	const FString Email    = EmailBox    ? EmailBox->GetText().ToString()    : FString();
	const FString Password = PasswordBox ? PasswordBox->GetText().ToString() : FString();
	if (Email.IsEmpty() || Password.IsEmpty())
	{
		if (StatusText) { StatusText->SetText(FText::FromString(TEXT("email/password를 입력하세요"))); }
		return;
	}

	TWeakObjectPtr<UAuthWidgetBase> WeakThis(this);
	Auth->Login(Email, Password,
		[WeakThis](bool bOk, int32 Code, const FString& Content)
		{
			if (UAuthWidgetBase* Self = WeakThis.Get()) { Self->SetStatus(bOk, Code, Content); }
		});
}

void UAuthWidgetBase::OnMeClicked()
{
	UAuthSubsystem* Auth = GetAuth();
	if (!Auth) { return; }

	TWeakObjectPtr<UAuthWidgetBase> WeakThis(this);
	Auth->GetMe(
		[WeakThis](bool bOk, int32 Code, const FString& Content)
		{
			if (UAuthWidgetBase* Self = WeakThis.Get()) { Self->SetStatus(bOk, Code, Content); }
		});
}

void UAuthWidgetBase::OnDeleteClicked()
{
	UAuthSubsystem* Auth = GetAuth();
	if (!Auth) { return; }

	TWeakObjectPtr<UAuthWidgetBase> WeakThis(this);
	Auth->DeleteMe(
		[WeakThis](bool bOk, int32 Code, const FString& Content)
		{
			if (UAuthWidgetBase* Self = WeakThis.Get()) { Self->SetStatus(bOk, Code, Content); }
		});
}

void UAuthWidgetBase::OnHealthClicked()
{
	UAuthSubsystem* Auth = GetAuth();
	if (!Auth) { return; }

	TWeakObjectPtr<UAuthWidgetBase> WeakThis(this);
	Auth->Health(
		[WeakThis](bool bOk, int32 Code, const FString& Content)
		{
			if (UAuthWidgetBase* Self = WeakThis.Get()) { Self->SetStatus(bOk, Code, Content); }
		});
}
