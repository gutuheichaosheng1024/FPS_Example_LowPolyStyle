#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "FPSPlayerState.generated.h"

/**
 * AFPSPlayerState — 玩家状态，跨角色生命周期持久，存储计分与时间同步数据
 *
 * 职责：管理存活时间计时、Kills/Deaths/Score 统计、RemainingTime/GameStartTime/GameDuration 同步，
 *       所有属性自动 Replicated 到客户端供 UI 读取
 * 使用：无特定依赖（继承自 APlayerState）
 */
UCLASS()
class FPS_API AFPSPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	AFPSPlayerState();
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	float SpawnedTime = 0.f;

	UFUNCTION(BlueprintPure, Category = "Score")
	float GetSurvivalTime() const;

	void ResetSurvivalTime();

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Score")
	int32 Kills = 0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Score")
	int32 Deaths = 0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game")
	float RemainingTime = 0.f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game")
	float GameStartTime = 0.f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game")
	float GameDuration = 0.f;
};
