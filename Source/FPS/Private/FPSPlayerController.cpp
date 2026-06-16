#include "FPSPlayerController.h"
#include "FPSGameMode.h"
#include "UI/FPSRespawnWidget.h"
#include "UI/FPSGameEndWidget.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/Pawn.h"

// ======================================================================
// Client RPC
// ======================================================================

void AFPSPlayerController::Client_ShowRespawnUI_Implementation(const FString& KillerName)
{
	if (!IsLocalController()) return;
	ShowRespawnUI(KillerName);
}

void AFPSPlayerController::Client_GameEnd_Implementation(
	const FString& Top1Name, float Top1Score, int32 Top1Kills,
	const FString& Top2Name, float Top2Score, int32 Top2Kills,
	const FString& Top3Name, float Top3Score, int32 Top3Kills)
{
	if (!GameEndUIClass) return;

	UUserWidget* GameEndUI = CreateWidget<UUserWidget>(this, GameEndUIClass);
	if (!GameEndUI) return;

	// 尝试C++接口设置数据
	if (UFPSGameEndWidget* EndWidget = Cast<UFPSGameEndWidget>(GameEndUI))
	{
		EndWidget->UpdateScoreboard(
			Top1Name, Top1Score, Top1Kills,
			Top2Name, Top2Score, Top2Kills,
			Top3Name, Top3Score, Top3Kills);
	}

	GameEndUI->AddToViewport(200);
	SetInputMode(FInputModeUIOnly());
	SetShowMouseCursor(true);
}

// ======================================================================
// Server RPC
// ======================================================================

bool AFPSPlayerController::Server_RequestRespawn_Validate()
{
	return true;
}

void AFPSPlayerController::Server_RequestRespawn_Implementation()
{
	HideRespawnUI();

	// 恢复游戏输入模式
	SetInputMode(FInputModeGameOnly());
	SetShowMouseCursor(false);

	// 销毁旧 Pawn 残留体
	if (APawn* PrevPawn = GetPawn())
	{
		PrevPawn->Destroy();
	}

	// 引擎标准重生流程
	if (AFPSGameMode* GM = Cast<AFPSGameMode>(GetWorld()->GetAuthGameMode()))
	{
		GM->RestartPlayer(this);
	}
}

// ======================================================================
// 内部方法
// ======================================================================

void AFPSPlayerController::ShowRespawnUI(const FString& KillerName)
{
	if (!RespawnUIClass) return;

	HideRespawnUI();

	RespawnUIInstance = CreateWidget<UUserWidget>(this, RespawnUIClass);
	if (!RespawnUIInstance) return;

	// 设置击杀者名称
	if (UFPSRespawnWidget* RespawnWidget = Cast<UFPSRespawnWidget>(RespawnUIInstance))
	{
		RespawnWidget->SetKillerName(KillerName);
	}

	RespawnUIInstance->AddToViewport(100);
	SetInputMode(FInputModeUIOnly());
	SetShowMouseCursor(true);
}

void AFPSPlayerController::HideRespawnUI()
{
	if (RespawnUIInstance)
	{
		RespawnUIInstance->RemoveFromParent();
		RespawnUIInstance = nullptr;
	}
}
