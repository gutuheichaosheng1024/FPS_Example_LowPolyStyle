#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "FPSPlayerController.generated.h"

/**
 * 游戏内 PlayerController（与主菜单 AFPSMainMenuPlayerController 独立）
 *
 * 职责：
 * - 接收服务器推送的 Client RPC（显示重生UI / 游戏结束UI）
 * - 发送 Server RPC（请求重生）
 * - UI 实例管理（跨角色生命周期持久，不受 Pawn 销毁影响）
 */
class AFPS_CharacterBase;
class AWeaponActor;
class UFPSHUDWidget;
class USoundBase;

UCLASS()
class FPS_API AFPSPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	// ---------- 战斗 HUD（蓝图创建 HUD 时赋值）----------

	/** 战斗 HUD 引用（蓝图创建 HUD 后赋值，Client RPC 用此显示命中/击杀反馈） */
	UPROPERTY(BlueprintReadWrite, Category = "UI")
	TObjectPtr<UFPSHUDWidget> CachedHUDWidget;

	// ---------- UI 类配置（蓝图中设置）----------

	/** 重生界面 Widget 类 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UUserWidget> RespawnUIClass;

	/** 游戏结束界面 Widget 类 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UUserWidget> GameEndUIClass;

	// ---------- Client RPC（服务器→客户端）----------

	/** 服务器推送：显示重生UI */
	UFUNCTION(Client, Reliable)
	void Client_ShowRespawnUI(const FString& KillerName);

	/** 服务器推送：游戏结束，显示计分板 */
	UFUNCTION(Client, Reliable)
	void Client_GameEnd(
		const FString& Top1Name, float Top1Score, int32 Top1Kills,
		const FString& Top2Name, float Top2Score, int32 Top2Kills,
		const FString& Top3Name, float Top3Score, int32 Top3Kills);

	// ---------- Server RPC（客户端→服务器）----------

	/** 客户端请求重生 */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestRespawn();

	// ---------- 命中/击杀反馈 Client RPC ----------

	/** 服务器推送：命中确认（PlayerController 通过委托订阅触发） */
	UFUNCTION(Client, Reliable)
	void Client_OnHitConfirmed(bool bKilled);

	/** 服务器推送：击杀确认（PlayerController 通过委托订阅触发） */
	UFUNCTION(Client, Reliable)
	void Client_OnKillConfirmed();

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// ---------- 委托绑定管理 ----------

	/** 绑定角色的击杀委托 */
	void BindCharacterDelegates(AFPS_CharacterBase* InCharacter);

	/** 解绑角色的击杀委托 */
	void UnbindCharacterDelegates();

	/** 绑定武器的命中委托 */
	void BindWeaponHitDelegate(AWeaponActor* Weapon);

	/** 解绑武器的命中委托 */
	void UnbindWeaponHitDelegate();

	// ---------- 委托回调 ----------

	/** 武器命中回调（服务器端）→ 转发 Client RPC */
	UFUNCTION()
	void HandleHitConfirmed(bool bKilled);

	/** 武器切换回调 → 重新绑定命中委托 */
	UFUNCTION()
	void OnWeaponActivatedHandler(AWeaponActor* NewWeapon);

	/** 延迟绑定武器委托（等待 BeginPlay 生成武器后调用） */
	void BindWeaponAfterRespawn();

	// ---------- 重生UI内部方法 ----------

	/** 创建并显示重生UI */
	void ShowRespawnUI(const FString& KillerName);

	/** 隐藏并销毁重生UI */
	void HideRespawnUI();

	// ---------- 状态 ----------

	/** 当前重生UI实例 */
	UPROPERTY()
	TObjectPtr<UUserWidget> RespawnUIInstance;

	/** 缓存的角色引用（用于解绑委托） */
	UPROPERTY()
	TObjectPtr<AFPS_CharacterBase> CachedCharacter;

	/** 缓存的武器引用（用于解绑命中委托） */
	UPROPERTY()
	TObjectPtr<AWeaponActor> CachedWeapon;
};
