#include "AuthSubsystem.h"
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
