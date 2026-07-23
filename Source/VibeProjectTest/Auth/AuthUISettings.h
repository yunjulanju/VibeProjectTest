#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "AuthUISettings.generated.h"

class UUserWidget;

/**
 * Play(PIE) 시작 시 자동 표시할 Auth 위젯(WBP)을 Project Settings에서 지정한다.
 * Project Settings > Game > Auth UI 에 노출된다.
 */
UCLASS(config = Game, defaultconfig, meta = (DisplayName = "Auth UI"))
class VIBEPROJECTTEST_API UAuthUISettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** 뷰포트에 추가할 위젯 클래스. AuthWidgetBase로 reparent한 WBP를 지정한다. 미지정 시 UI 미표시. */
	UPROPERTY(config, EditAnywhere, Category = "Auth UI", meta = (AllowAbstract = false))
	TSoftClassPtr<UUserWidget> AuthWidgetClass;

	virtual FName GetCategoryName() const override { return TEXT("Game"); }
};
