#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AuthWidgetBase.generated.h"

class UButton;
class UEditableTextBox;
class UTextBlock;
class UAuthSubsystem;

/**
 * FastAPI 인증 서버를 조작하는 화면 UI.
 * WBP를 이 클래스로 reparent하고 아래 BindWidget 이름들로 위젯을 배치한다(레이아웃은 자유).
 */
UCLASS()
class VIBEPROJECTTEST_API UAuthWidgetBase : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	// --- 입력란 (WBP에서 정확한 이름으로 배치) ---
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UEditableTextBox> EmailBox;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UEditableTextBox> UsernameBox;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UEditableTextBox> PasswordBox;

	// --- 버튼 ---
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> SignupButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> LoginButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> MeButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> DeleteButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> HealthButton;

	// --- 표시 ---
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> StatusText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> LoginStateText;

	UFUNCTION() void OnSignupClicked();
	UFUNCTION() void OnLoginClicked();
	UFUNCTION() void OnMeClicked();
	UFUNCTION() void OnDeleteClicked();
	UFUNCTION() void OnHealthClicked();

private:
	UAuthSubsystem* GetAuth() const;
	void RefreshButtons();
	/** 콜백 결과를 StatusText에 표시하고 버튼 상태를 갱신한다. */
	void SetStatus(bool bSuccess, int32 StatusCode, const FString& Content);
};
