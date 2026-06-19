#include "FPSGameMode.h"
#include "FPSPlayerState.h"
#include "FPSPlayerController.h"
#include "Character/FPS_CharacterBase.h"
#include "Character/FPS_AICharacter.h"
#include "UI/FPSSaveGame.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"

AFPSGameMode::AFPSGameMode()
{
	bReplicates = true;
	PlayerStateClass = AFPSPlayerState::StaticClass();
}

void AFPSGameMode::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AFPSGameMode, RemainingTime);
	DOREPLIFETIME(AFPSGameMode, bGameActive);
}

void AFPSGameMode::BeginPlay()
{
	Super::BeginPlay();

	// 初始化 AI 名称池
	AvailableAINames = AINames;

	// 启动游戏倒计时
	bGameActive = true;
	GameStartTime = GetWorld()->GetTimeSeconds();
	RemainingTime = GameDuration;
	GetWorldTimerManager().SetTimer(GameTimerHandle, this, &AFPSGameMode::TickGameTimer, 0.1f, true);

	// 同步时间戳到已连接的玩家
	SyncGameTimeToAllPlayers();
}

// ======================================================================
// 引擎覆盖
// ======================================================================

void AFPSGameMode::RestartPlayer(AController* NewPlayer)
{
	// 引擎标准重生：找PlayerStart → SpawnActor(DefaultPawnClass) → Possess
	Super::RestartPlayer(NewPlayer);

	// 重置存活计时（PlayerState跨角色生命周期持久）
	if (AFPSPlayerState* PS = NewPlayer->GetPlayerState<AFPSPlayerState>())
	{
		PS->ResetSurvivalTime();

		// 同步游戏时间到新玩家（可能中途加入）
		SyncGameTimeToPlayer(PS);
	}

	// 绑定新角色的死亡委托
	if (APawn* NewPawn = NewPlayer->GetPawn())
	{
		BindCharacterDelegates(NewPawn);
	}
}

void AFPSGameMode::OnPostLogin(AController* NewPlayer)
{
	Super::OnPostLogin(NewPlayer);

	if (!NewPlayer) return;

	// 从存档写入 PlayerName 到 PlayerState（首次进入游戏时）
	if (APlayerState* PS = NewPlayer->GetPlayerState<APlayerState>())
	{
		const FString SavedName = UFPSSaveGame::LoadPlayerName();
		PS->SetPlayerName(SavedName);
	}
}

void AFPSGameMode::BindCharacterDelegates(APawn* Pawn)
{
	if (AFPS_CharacterBase* Char = Cast<AFPS_CharacterBase>(Pawn))
	{
		Char->OnKilled.AddDynamic(this, &AFPSGameMode::OnCharacterKilled);
	}
}

// ======================================================================
// 计时
// ======================================================================

void AFPSGameMode::TickGameTimer()
{
	if (!bGameActive) return;

	// 计算剩余时间（基于服务器时间戳，精度更高）
	const float Elapsed = GetWorld()->GetTimeSeconds() - GameStartTime;
	RemainingTime = FMath::Max(GameDuration - Elapsed, 0.f);

	if (RemainingTime <= 0.f)
	{
		RemainingTime = 0.f;
		EndGame();
	}
}

// ======================================================================
// 击杀 + 计分
// ======================================================================

void AFPSGameMode::OnCharacterKilled(AFPS_CharacterBase* Victim, AController* KillerController)
{
	if (!Victim || !bGameActive) return;

	AController* VictimController = Victim->GetController();

	RegisterKill(KillerController, VictimController);

	// 通知击杀者客户端显示击杀指示器
	if (KillerController && KillerController != VictimController)
	{
		if (AFPSPlayerController* KillerPC = Cast<AFPSPlayerController>(KillerController))
		{
			KillerPC->Client_OnKillConfirmed();
		}
	}

	// 分支：AI vs 玩家
	if (Cast<AFPS_AICharacter>(Victim))
	{
		// AI：调度重生，归还名称，延迟销毁（保留布娃娃表现）
		ScheduleAIRespawn(VictimController);

		if (AFPSPlayerState* PS = VictimController ? VictimController->GetPlayerState<AFPSPlayerState>() : nullptr)
		{
			ReturnAIName(PS->GetPlayerName());
		}

		// 短延迟销毁（让布娃娃可见）
		FTimerHandle DestroyHandle;
		FTimerDelegate DestroyDelegate;
		DestroyDelegate.BindLambda([Victim]()
		{
			if (IsValid(Victim)) Victim->Destroy();
		});
		GetWorldTimerManager().SetTimer(DestroyHandle, DestroyDelegate, 2.f, false);
	}
	else
	{
		// 玩家：延迟销毁（保留布娃娃表现）
		FTimerHandle DestroyHandle;
		FTimerDelegate DestroyDelegate;
		DestroyDelegate.BindLambda([Victim]()
		{
			if (IsValid(Victim))
			{
				Victim->Destroy();
			}
		});
		GetWorldTimerManager().SetTimer(DestroyHandle, DestroyDelegate, PlayerRespawnDelay, false);

		// 推送重生UI（带击杀者名称）
		if (AFPSPlayerController* PC = Cast<AFPSPlayerController>(VictimController))
		{
			FString KillerName = TEXT("Unknown");
			if (KillerController)
			{
				if (APlayerState* KillerPS = KillerController->GetPlayerState<APlayerState>())
				{
					KillerName = KillerPS->GetPlayerName();
				}
			}

			PC->Client_ShowRespawnUI(KillerName);
		}
	}
}

void AFPSGameMode::RegisterKill(AController* Killer, AController* Victim)
{
	if (!Victim) return;

	// 受害者的计分数据（确保存在）
	int32* VictimIdxPtr = ControllerScoreIndex.Find(Victim);
	if (!VictimIdxPtr)
	{
		FPlayerScoreData NewData;
		NewData.PlayerName = GetControllerPlayerName(Victim);
		int32 NewIdx = Scoreboard.Add(NewData);
		VictimIdxPtr = &ControllerScoreIndex.Add(Victim, NewIdx);
	}
	FPlayerScoreData& VictimData = Scoreboard[*VictimIdxPtr];
	VictimData.Deaths++;

	// 同步到 PlayerState
	if (AFPSPlayerState* VictimPS = Victim->GetPlayerState<AFPSPlayerState>())
	{
		VictimPS->Deaths = VictimData.Deaths;
	}

	if (!Killer || Killer == Victim) return;

	// 击杀者的计分数据（确保存在）
	int32* KillerIdxPtr = ControllerScoreIndex.Find(Killer);
	if (!KillerIdxPtr)
	{
		FPlayerScoreData NewData;
		NewData.PlayerName = GetControllerPlayerName(Killer);
		int32 NewIdx = Scoreboard.Add(NewData);
		KillerIdxPtr = &ControllerScoreIndex.Add(Killer, NewIdx);
	}
	FPlayerScoreData& KillerData = Scoreboard[*KillerIdxPtr];
	KillerData.Kills++;

	// 记录击杀者名称（受害者的重生UI用）
	VictimData.LastKillerName = KillerData.PlayerName;

	// 计分公式：
	// Score = BaseKillScore + (VictimSurvivalTime × SurvivalTimeWeight) + (Killer总击杀数 × KillCountWeight)
	// 注：KillCountWeight基于总击杀数(滚雪球)，非连杀数
	float VictimSurvivalTime = 0.f;
	if (AFPSPlayerState* VictimPS = Victim->GetPlayerState<AFPSPlayerState>())
	{
		VictimSurvivalTime = VictimPS->GetSurvivalTime();
	}

	float KillScore = BaseKillScore
		+ VictimSurvivalTime * SurvivalTimeWeight
		+ static_cast<float>(KillerData.Kills) * KillCountWeight;

	KillerData.TotalScore += KillScore;

	// 同步到 PlayerState（自动 Replicated，客户端UI可读）
	if (AFPSPlayerState* KillerPS = Killer->GetPlayerState<AFPSPlayerState>())
	{
		KillerPS->Kills = KillerData.Kills;
		KillerPS->SetScore(KillerData.TotalScore);
	}
}

// ======================================================================
// AI 重生
// ======================================================================

void AFPSGameMode::ScheduleAIRespawn(AController* AICon)
{
	if (!AICon || !bGameActive) return;

	FTimerHandle& Handle = RespawnTimers.FindOrAdd(AICon);
	FTimerDelegate TimerDelegate;
	TimerDelegate.BindUObject(this, &AFPSGameMode::RespawnAI, AICon);
	GetWorldTimerManager().SetTimer(Handle, TimerDelegate, AIRespawnDelay, false);
}

void AFPSGameMode::RespawnAI(AController* AICon)
{
	if (!AICon || !bGameActive) return;

	RespawnTimers.Remove(AICon);

	// 标准重生
	RestartPlayer(AICon);

	// 分配新名称
	if (AFPSPlayerState* PS = AICon->GetPlayerState<AFPSPlayerState>())
	{
		PS->SetPlayerName(ClaimAIName());
	}
}

// ======================================================================
// 游戏结束
// ======================================================================

void AFPSGameMode::EndGame()
{
	bGameActive = false;
	GetWorldTimerManager().ClearTimer(GameTimerHandle);

	// 清除所有待定重生定时器
	for (auto& Pair : RespawnTimers)
	{
		GetWorldTimerManager().ClearTimer(Pair.Value);
	}
	RespawnTimers.Empty();

	// 排序计分板（降序）
	Scoreboard.Sort([](const FPlayerScoreData& A, const FPlayerScoreData& B) {
		return A.TotalScore > B.TotalScore;
	});

	// 给每个玩家客户端推送游戏结束UI
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (AFPSPlayerController* FPSPC = Cast<AFPSPlayerController>(It->Get()))
		{
			FPlayerScoreData Top1 = Scoreboard.Num() > 0 ? Scoreboard[0] : FPlayerScoreData();
			FPlayerScoreData Top2 = Scoreboard.Num() > 1 ? Scoreboard[1] : FPlayerScoreData();
			FPlayerScoreData Top3 = Scoreboard.Num() > 2 ? Scoreboard[2] : FPlayerScoreData();

			FPSPC->Client_GameEnd(
				Top1.PlayerName, Top1.TotalScore, Top1.Kills,
				Top2.PlayerName, Top2.TotalScore, Top2.Kills,
				Top3.PlayerName, Top3.TotalScore, Top3.Kills);
		}
	}
}

// ======================================================================
// 时间同步
// ======================================================================

void AFPSGameMode::SyncGameTimeToAllPlayers()
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (PC)
		{
			if (AFPSPlayerState* PS = PC->GetPlayerState<AFPSPlayerState>())
			{
				SyncGameTimeToPlayer(PS);
			}
		}
	}
}

void AFPSGameMode::SyncGameTimeToPlayer(AFPSPlayerState* PS)
{
	if (!PS) return;
	PS->GameStartTime = GameStartTime;
	PS->GameDuration = GameDuration;
}

TArray<FPlayerScoreData> AFPSGameMode::GetTopPlayers(int32 Count) const
{
	TArray<FPlayerScoreData> Sorted = Scoreboard;
	Sorted.Sort([](const FPlayerScoreData& A, const FPlayerScoreData& B) {
		return A.TotalScore > B.TotalScore;
	});

	if (Sorted.Num() > Count)
	{
		Sorted.SetNum(Count);
	}
	return Sorted;
}

// ======================================================================
// AI 名称管理
// ======================================================================

FString AFPSGameMode::ClaimAIName()
{
	if (AvailableAINames.Num() == 0)
	{
		static int32 FallbackIndex = 0;
		return FString::Printf(TEXT("Bot_%d"), ++FallbackIndex);
	}

	int32 Idx = FMath::RandRange(0, AvailableAINames.Num() - 1);
	FString Name = AvailableAINames[Idx];
	AvailableAINames.RemoveAt(Idx);
	return Name;
}

void AFPSGameMode::ReturnAIName(const FString& Name)
{
	if (!AvailableAINames.Contains(Name) && AINames.Contains(Name))
	{
		AvailableAINames.Add(Name);
	}
}

// ======================================================================
// 工具
// ======================================================================

FString AFPSGameMode::GetControllerPlayerName(AController* Ctrl) const
{
	if (!Ctrl) return TEXT("Unknown");

	if (APlayerState* PS = Ctrl->GetPlayerState<APlayerState>())
	{
		const FString Name = PS->GetPlayerName();
		if (!Name.IsEmpty())
		{
			return Name;
		}
	}

	return TEXT("Unknown");
}
