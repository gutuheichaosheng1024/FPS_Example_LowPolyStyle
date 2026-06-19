#include "FPSPlayerController.h"
#include "FPSGameMode.h"
#include "Character/FPS_CharacterBase.h"
#include "Weapon/WeaponActor.h"
#include "Weapon/WeaponAudioConfig.h"
#include "UI/FPSHUDWidget.h"
#include "UI/FPSRespawnWidget.h"
#include "UI/FPSGameEndWidget.h"
#include "Blueprint/UserWidget.h"

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

// ======================================================================
// 生命周期
// ======================================================================

void AFPSPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// 解绑旧角色的委托
	UnbindCharacterDelegates();
	UnbindWeaponHitDelegate();

	// 绑定新角色的委托
	if (AFPS_CharacterBase* Char = Cast<AFPS_CharacterBase>(InPawn))
	{
		CachedCharacter = Char;
		BindCharacterDelegates(Char);

		// 监听武器切换，重新绑定命中委托
		Char->OnWeaponActivated.AddDynamic(this, &AFPSPlayerController::OnWeaponActivatedHandler);

		// 延迟绑定武器（等待 BeginPlay 生成武器）
		GetWorldTimerManager().SetTimerForNextTick(this, &AFPSPlayerController::BindWeaponAfterRespawn);
	}
}

void AFPSPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindCharacterDelegates();
	UnbindWeaponHitDelegate();
	Super::EndPlay(EndPlayReason);
}

// ======================================================================
// 委托绑定管理
// ======================================================================

void AFPSPlayerController::BindCharacterDelegates(AFPS_CharacterBase* InCharacter)
{
	if (!InCharacter) return;
	// 不再订阅 OnKilled — 击杀通知由 GameMode::OnCharacterKilled 直接调用 Client_OnKillConfirmed
}

void AFPSPlayerController::UnbindCharacterDelegates()
{
	if (CachedCharacter)
	{
		CachedCharacter->OnWeaponActivated.RemoveDynamic(this, &AFPSPlayerController::OnWeaponActivatedHandler);
		CachedCharacter = nullptr;
	}
}

void AFPSPlayerController::BindWeaponHitDelegate(AWeaponActor* Weapon)
{
	if (!Weapon) return;
	Weapon->OnHitConfirmed.AddDynamic(this, &AFPSPlayerController::HandleHitConfirmed);
}

void AFPSPlayerController::UnbindWeaponHitDelegate()
{
	if (CachedWeapon)
	{
		CachedWeapon->OnHitConfirmed.RemoveDynamic(this, &AFPSPlayerController::HandleHitConfirmed);
		CachedWeapon = nullptr;
	}
}

// ======================================================================
// 委托回调 → Client RPC
// ======================================================================

void AFPSPlayerController::HandleHitConfirmed(bool bKilled)
{
	// 服务器端回调，转发到客户端
	Client_OnHitConfirmed(bKilled);
}

void AFPSPlayerController::OnWeaponActivatedHandler(AWeaponActor* NewWeapon)
{
	// 武器切换：解绑旧武器，绑定新武器
	UnbindWeaponHitDelegate();
	CachedWeapon = NewWeapon;
	BindWeaponHitDelegate(NewWeapon);
}

void AFPSPlayerController::BindWeaponAfterRespawn()
{
	UnbindWeaponHitDelegate();
	if (CachedCharacter && CachedCharacter->CurrentWeapon)
	{
		CachedWeapon = CachedCharacter->CurrentWeapon;
		BindWeaponHitDelegate(CachedWeapon);

		// 重新绑定武器输入（BeginPlay 时 Pawn 尚未 Possess，BindInput 失败，需要在 Possess 后重试）
		// 先清除旧的 CachedInputComponent，确保 TryBindInput 不被阻挡
		CachedWeapon->SetWeaponActionsLocked(false);
		CachedWeapon->TryBindInput();
	}
}

// ======================================================================
// 命中/击杀反馈 Client RPC
// ======================================================================

void AFPSPlayerController::Client_OnHitConfirmed_Implementation(bool bKilled)
{
	if (!IsLocalController() || !CachedHUDWidget) return;

	CachedHUDWidget->ShowHitMarker();

	// 获取武器引用（CachedWeapon 仅服务端设置，客户端需要回退到 GetPawn）
	AWeaponActor* Weapon = CachedWeapon.Get();
	if (!Weapon)
	{
		if (AFPS_CharacterBase* Char = Cast<AFPS_CharacterBase>(GetPawn()))
			Weapon = Char->CurrentWeapon;
	}
	if (Weapon)
	{
		if (UWeaponAudioConfig* Audio = Weapon->GetAudioConfig())
			CachedHUDWidget->PlayHitConfirmSound(Audio->HitConfirmSound);
	}
}

void AFPSPlayerController::Client_OnKillConfirmed_Implementation()
{
	if (!IsLocalController() || !CachedHUDWidget) return;

	CachedHUDWidget->ShowKillIndicator();

	AWeaponActor* Weapon = CachedWeapon.Get();
	if (!Weapon)
	{
		if (AFPS_CharacterBase* Char = Cast<AFPS_CharacterBase>(GetPawn()))
			Weapon = Char->CurrentWeapon;
	}
	if (Weapon)
	{
		if (UWeaponAudioConfig* Audio = Weapon->GetAudioConfig())
			CachedHUDWidget->PlayKillConfirmSound(Audio->KillConfirmSound);
	}
}
