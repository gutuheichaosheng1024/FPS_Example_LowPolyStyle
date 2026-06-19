#include "FPSPlayerState.h"
#include "Net/UnrealNetwork.h"

AFPSPlayerState::AFPSPlayerState()
{
	bReplicates = true;
}

void AFPSPlayerState::BeginPlay()
{
	Super::BeginPlay();
	SpawnedTime = GetWorld()->GetTimeSeconds();
}

void AFPSPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AFPSPlayerState, Kills);
	DOREPLIFETIME(AFPSPlayerState, Deaths);
	DOREPLIFETIME(AFPSPlayerState, RemainingTime);
	DOREPLIFETIME(AFPSPlayerState, GameStartTime);
	DOREPLIFETIME(AFPSPlayerState, GameDuration);
	// Score 继承自 APlayerState，基类已处理 Replicated
}

float AFPSPlayerState::GetSurvivalTime() const
{
	if (const UWorld* World = GetWorld())
	{
		return World->GetTimeSeconds() - SpawnedTime;
	}
	return 0.f;
}

void AFPSPlayerState::ResetSurvivalTime()
{
	if (UWorld* World = GetWorld())
	{
		SpawnedTime = World->GetTimeSeconds();
	}
}
