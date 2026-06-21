#include "FPSPlayerController.h"
#include "FPSGameMode.h"
#include "Character/FPS_CharacterBase.h"
#include "Weapon/WeaponActor.h"
#include "Weapon/WeaponAudioConfig.h"
#include "UI/FPSHUDWidget.h"
#include "UI/FPSRespawnWidget.h"
#include "UI/FPSGameEndWidget.h"
#include "Blueprint/UserWidget.h"

// Client_ShowRespawnUI_Implementation：服务器推送重生 UI 到客户端
// 流程：检查 IsLocalController → 调用 ShowRespawnUI 创建并显示重生界面
void AFPSPlayerController::Client_ShowRespawnUI_Implementation(const FString& KillerName)
{
	if (!IsLocalController()) return;
	ShowRespawnUI(KillerName);
}

// Client_GameEnd_Implementation：服务器推送游戏结束结算 UI 到客户端
// 流程：检查 GameEndUIClass → CreateWidget → 尝试 Cast UFPSGameEndWidget 设置计分板数据 → AddToViewport → 切换 UIOnly 输入模式
void AFPSPlayerController::Client_GameEnd_Implementation(
	const FString& Top1Name, float Top1Score, int32 Top1Kills,
	const FString& Top2Name, float Top2Score, int32 Top2Kills,
	const FString& Top3Name, float Top3Score, int32 Top3Kills)
{
	if (!GameEndUIClass) return;

	UUserWidget* GameEndUI = CreateWidget<UUserWidget>(this, GameEndUIClass);
	if (!GameEndUI) return;

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

// Server_RequestRespawn_Validate：重生请求验证，始终允许
// 流程：返回 true
bool AFPSPlayerController::Server_RequestRespawn_Validate()
{
	return true;
}

// Server_RequestRespawn_Implementation：客户端请求重生，销毁旧 Pawn 并调用 GameMode::RestartPlayer
// 流程：HideRespawnUI → 恢复 GameOnly 输入模式 → 销毁旧 Pawn → 调用 GM->RestartPlayer
void AFPSPlayerController::Server_RequestRespawn_Implementation()
{
	HideRespawnUI();

	SetInputMode(FInputModeGameOnly());
	SetShowMouseCursor(false);

	if (APawn* PrevPawn = GetPawn())
	{
		PrevPawn->Destroy();
	}

	if (AFPSGameMode* GM = Cast<AFPSGameMode>(GetWorld()->GetAuthGameMode()))
	{
		GM->RestartPlayer(this);
	}
}

// ShowRespawnUI：创建并显示重生界面，设置击杀者名称
// 流程：检查 RespawnUIClass → HideRespawnUI 清理旧实例 → CreateWidget → 尝试设置击杀者名 → AddToViewport → UIOnly 输入模式
void AFPSPlayerController::ShowRespawnUI(const FString& KillerName)
{
	if (!RespawnUIClass) return;

	HideRespawnUI();

	RespawnUIInstance = CreateWidget<UUserWidget>(this, RespawnUIClass);
	if (!RespawnUIInstance) return;

	if (UFPSRespawnWidget* RespawnWidget = Cast<UFPSRespawnWidget>(RespawnUIInstance))
	{
		RespawnWidget->SetKillerName(KillerName);
	}

	RespawnUIInstance->AddToViewport(100);
	SetInputMode(FInputModeUIOnly());
	SetShowMouseCursor(true);
}

// HideRespawnUI：隐藏并销毁当前重生 UI 实例
// 流程：检查 RespawnUIInstance → RemoveFromParent → 置空
void AFPSPlayerController::HideRespawnUI()
{
	if (RespawnUIInstance)
	{
		RespawnUIInstance->RemoveFromParent();
		RespawnUIInstance = nullptr;
	}
}

// OnPossess：Possess 新 Pawn 时绑定角色/武器委托
// 流程：调用 Super → 解绑旧委托 → Cast 新 Pawn → 绑定角色委托 → 订阅武器切换事件 → SetTimerForNextTick 延迟绑定武器
void AFPSPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	UnbindCharacterDelegates();
	UnbindWeaponHitDelegate();

	if (AFPS_CharacterBase* Char = Cast<AFPS_CharacterBase>(InPawn))
	{
		CachedCharacter = Char;
		BindCharacterDelegates(Char);

		Char->OnWeaponActivated.AddDynamic(this, &AFPSPlayerController::OnWeaponActivatedHandler);

		GetWorldTimerManager().SetTimerForNextTick(this, &AFPSPlayerController::BindWeaponAfterRespawn);
	}
}

// EndPlay：结束时解绑所有委托
// 流程：UnbindCharacterDelegates → UnbindWeaponHitDelegate → Super::EndPlay
void AFPSPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindCharacterDelegates();
	UnbindWeaponHitDelegate();
	Super::EndPlay(EndPlayReason);
}

// BindCharacterDelegates：绑定角色的委托（当前为空实现，击杀通知由 GameMode 直接调用 Client RPC）
// 流程：检查参数有效性
void AFPSPlayerController::BindCharacterDelegates(AFPS_CharacterBase* InCharacter)
{
	if (!InCharacter) return;
}

// UnbindCharacterDelegates：解绑旧角色的武器切换委托并清除缓存
// 流程：检查 CachedCharacter → 解绑 OnWeaponActivated → 置空 CachedCharacter
void AFPSPlayerController::UnbindCharacterDelegates()
{
	if (CachedCharacter)
	{
		CachedCharacter->OnWeaponActivated.RemoveDynamic(this, &AFPSPlayerController::OnWeaponActivatedHandler);
		CachedCharacter = nullptr;
	}
}

// BindWeaponHitDelegate：绑定武器的命中确认委托
// 流程：检查 Weapon 有效性 → 绑定 HandleHitConfirmed
void AFPSPlayerController::BindWeaponHitDelegate(AWeaponActor* Weapon)
{
	if (!Weapon) return;
	Weapon->OnHitConfirmed.AddDynamic(this, &AFPSPlayerController::HandleHitConfirmed);
}

// UnbindWeaponHitDelegate：解绑旧武器的命中委托并清除缓存
// 流程：检查 CachedWeapon → 解绑 OnHitConfirmed → 置空 CachedWeapon
void AFPSPlayerController::UnbindWeaponHitDelegate()
{
	if (CachedWeapon)
	{
		CachedWeapon->OnHitConfirmed.RemoveDynamic(this, &AFPSPlayerController::HandleHitConfirmed);
		CachedWeapon = nullptr;
	}
}

// HandleHitConfirmed：服务器端命中回调，转发 Client RPC 到客户端
// 流程：直接调用 Client_OnHitConfirmed(bKilled)
void AFPSPlayerController::HandleHitConfirmed(bool bKilled)
{
	Client_OnHitConfirmed(bKilled);
}

// OnWeaponActivatedHandler：武器切换时重新绑定命中委托到新武器
// 流程：解绑旧武器 → 缓存新武器 → 绑定新武器命中委托
void AFPSPlayerController::OnWeaponActivatedHandler(AWeaponActor* NewWeapon)
{
	UnbindWeaponHitDelegate();
	CachedWeapon = NewWeapon;
	BindWeaponHitDelegate(NewWeapon);
}

// BindWeaponAfterRespawn：Possess 后延迟绑定武器委托与输入
// 流程：解绑旧武器 → 从 CachedCharacter->CurrentWeapon 获取当前武器 → 绑定命中委托 → TryBindInput 重试输入绑定
void AFPSPlayerController::BindWeaponAfterRespawn()
{
	UnbindWeaponHitDelegate();
	if (CachedCharacter && CachedCharacter->CurrentWeapon)
	{
		CachedWeapon = CachedCharacter->CurrentWeapon;
		BindWeaponHitDelegate(CachedWeapon);

		CachedWeapon->SetWeaponActionsLocked(false);
		CachedWeapon->TryBindInput();
	}
}

// Client_OnHitConfirmed_Implementation：客户端命中反馈，显示命中标记并播放音效
// 流程：检查 IsLocalController 和 CachedHUDWidget → ShowHitMarker → 获取武器引用（CachedWeapon 或 Pawn->CurrentWeapon）→ 播放命中音效
void AFPSPlayerController::Client_OnHitConfirmed_Implementation(bool bKilled)
{
	if (!IsLocalController() || !CachedHUDWidget) return;

	CachedHUDWidget->ShowHitMarker();

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

// Client_OnKillConfirmed_Implementation：客户端击杀反馈，显示击杀指示器并播放音效
// 流程：检查 IsLocalController 和 CachedHUDWidget → ShowKillIndicator → 获取武器引用 → 播放击杀音效
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
