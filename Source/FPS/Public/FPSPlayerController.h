#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "FPSPlayerController.generated.h"

class AFPS_CharacterBase;
class AWeaponActor;
class UFPSHUDWidget;
class USoundBase;

/**
 * AFPSPlayerController — 游戏内 PlayerController，处理 Server/Client RPC 与 UI/委托管理
 *
 * 职责：接收服务器推送 Client RPC（命中/击杀/重生/游戏结束 UI）、发送 Server RPC（请求重生）、
 *       管理 HUD/CachedWeapon/CachedCharacter 引用与委托绑定/解绑、武器命中委托的跨武器切换绑定
 * 使用：AFPS_CharacterBase, AWeaponActor, UFPSHUDWidget, AFPSGameMode
 */
UCLASS()
class FPS_API AFPSPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "UI")
	TObjectPtr<UFPSHUDWidget> CachedHUDWidget;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UUserWidget> RespawnUIClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UUserWidget> GameEndUIClass;

	UFUNCTION(Client, Reliable)
	void Client_ShowRespawnUI(const FString& KillerName);

	UFUNCTION(Client, Reliable)
	void Client_GameEnd(
		const FString& Top1Name, float Top1Score, int32 Top1Kills,
		const FString& Top2Name, float Top2Score, int32 Top2Kills,
		const FString& Top3Name, float Top3Score, int32 Top3Kills);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestRespawn();

	UFUNCTION(Client, Reliable)
	void Client_OnHitConfirmed(bool bKilled);

	UFUNCTION(Client, Reliable)
	void Client_OnKillConfirmed();

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void BindCharacterDelegates(AFPS_CharacterBase* InCharacter);
	void UnbindCharacterDelegates();
	void BindWeaponHitDelegate(AWeaponActor* Weapon);
	void UnbindWeaponHitDelegate();

	UFUNCTION()
	void HandleHitConfirmed(bool bKilled);

	UFUNCTION()
	void OnWeaponActivatedHandler(AWeaponActor* NewWeapon);

	void BindWeaponAfterRespawn();

	void ShowRespawnUI(const FString& KillerName);
	void HideRespawnUI();

	UPROPERTY()
	TObjectPtr<UUserWidget> RespawnUIInstance;

	UPROPERTY()
	TObjectPtr<AFPS_CharacterBase> CachedCharacter;

	UPROPERTY()
	TObjectPtr<AWeaponActor> CachedWeapon;
};
