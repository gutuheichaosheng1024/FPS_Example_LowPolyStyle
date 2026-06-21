#include "FPSPlayerState.h"
#include "Net/UnrealNetwork.h"

// 构造函数：启用复制
// 流程：设置 bReplicates = true
AFPSPlayerState::AFPSPlayerState()
{
	bReplicates = true;
}

// BeginPlay：记录出生时间戳，用于后续存活时间计算
// 流程：调用 Super → 记录当前世界时间为 SpawnedTime
void AFPSPlayerState::BeginPlay()
{
	Super::BeginPlay();
	SpawnedTime = GetWorld()->GetTimeSeconds();
}

// GetLifetimeReplicatedProps：注册需要网络复制的属性
// 流程：调用 Super → 逐个注册 Kills/Deaths/RemainingTime/GameStartTime/GameDuration
void AFPSPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AFPSPlayerState, Kills);
	DOREPLIFETIME(AFPSPlayerState, Deaths);
	DOREPLIFETIME(AFPSPlayerState, RemainingTime);
	DOREPLIFETIME(AFPSPlayerState, GameStartTime);
	DOREPLIFETIME(AFPSPlayerState, GameDuration);
}

// GetSurvivalTime：计算当前存活时间（当前世界时间减去出生时间）
// 流程：获取 World → 返回 GetTimeSeconds() - SpawnedTime → 无 World 则返回 0
float AFPSPlayerState::GetSurvivalTime() const
{
	if (const UWorld* World = GetWorld())
	{
		return World->GetTimeSeconds() - SpawnedTime;
	}
	return 0.f;
}

// ResetSurvivalTime：重置存活计时，在重生时调用
// 流程：获取 World → 将 SpawnedTime 设为当前世界时间
void AFPSPlayerState::ResetSurvivalTime()
{
	if (UWorld* World = GetWorld())
	{
		SpawnedTime = World->GetTimeSeconds();
	}
}
