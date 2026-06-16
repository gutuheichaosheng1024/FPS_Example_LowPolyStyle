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
UCLASS()
class FPS_API AFPSPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
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

protected:
	// ---------- 内部方法 ----------

	/** 创建并显示重生UI */
	void ShowRespawnUI(const FString& KillerName);

	/** 隐藏并销毁重生UI */
	void HideRespawnUI();

	// ---------- 状态 ----------

	/** 当前重生UI实例 */
	UPROPERTY()
	TObjectPtr<UUserWidget> RespawnUIInstance;
};
