#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "FPSPlayerState.generated.h"

/**
 * 玩家状态（跨角色生命周期持久）
 * 存储存活时间、击杀/死亡/得分等计分数据，自动复制到客户端
 */
UCLASS()
class FPS_API AFPSPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	AFPSPlayerState();
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 本次出生时间戳（服务器权威，用于计算存活时间） */
	float SpawnedTime = 0.f;

	/** 获取当前存活时间（秒） */
	UFUNCTION(BlueprintPure, Category = "Score")
	float GetSurvivalTime() const;

	/** 重置存活计时（重生时调用） */
	void ResetSurvivalTime();

	// ---------- 计分属性（Replicated，客户端UI可读）----------
	// 注：Score 继承自 APlayerState（基类自带 Replicated），此处不重复声明

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Score")
	int32 Kills = 0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Score")
	int32 Deaths = 0;
};
