#include "FPSGameMode.h"
#include "FPSPlayerState.h"
#include "FPSPlayerController.h"
#include "AI/FPS_AIController.h"
#include "Character/FPS_CharacterBase.h"
#include "Character/FPS_AICharacter.h"
#include "UI/FPSSaveGame.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Online/OnlineSessionNames.h"
#include "Interfaces/OnlineSessionInterface.h"

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

	// 从 Session 读取最大玩家数
	ReadMaxPlayersFromSession();

	// 启动游戏倒计时
	bGameActive = true;
	GameStartTime = GetWorld()->GetTimeSeconds();
	RemainingTime = GameDuration;
	GetWorldTimerManager().SetTimer(GameTimerHandle, this, &AFPSGameMode::TickGameTimer, 0.1f, true);

	// 同步时间戳到已连接的玩家
	SyncGameTimeToAllPlayers();

	// AI 自动填充：游戏开始时补齐 AI
	if (bAutoFillWithAI && AIFillCharacterClass && AIFillControllerClass)
	{
		FillAIIfNeeded();
	}
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

AActor* AFPSGameMode::FindPlayerStart_Implementation(AController* Player, const FString& IncomingName)
{
	// 收集所有 PlayerStart
	TArray<AActor*> AllPlayerStarts;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), AllPlayerStarts);

	if (AllPlayerStarts.Num() > 0)
	{
		// 随机选择一个 PlayerStart
		int32 RandomIndex = FMath::RandRange(0, AllPlayerStarts.Num() - 1);
		return AllPlayerStarts[RandomIndex];
	}

	// 回退到引擎默认逻辑
	return Super::FindPlayerStart_Implementation(Player, IncomingName);
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

	// AI 自动填充：玩家加入时移除一个 AI
	if (bAutoFillWithAI)
	{
		RemoveAI(1);
	}
}

void AFPSGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	// AI 自动填充：玩家离开时延迟补充 AI（等待 GetNumPlayers 更新）
	if (bAutoFillWithAI)
	{
		FTimerHandle TempHandle;
		FTimerDelegate Del;
		Del.BindLambda([this]() { if (bGameActive) FillAIIfNeeded(); });
		GetWorldTimerManager().SetTimer(TempHandle, Del, 0.1f, false);
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

	// 同步到所有 PlayerState（GameMode 只存在于服务器，客户端通过 PlayerState 读取）
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (AFPSPlayerState* PS = It->Get()->GetPlayerState<AFPSPlayerState>())
		{
			PS->RemainingTime = RemainingTime;
		}
	}

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
		// AI：从 AIController 读取配置的重生时间
		AFPS_AIController* AICon = Cast<AFPS_AIController>(VictimController);
		float AIDelay = 5.f;
		if (AICon)
		{
			AIDelay = AICon->RespawnTime;

			// 记录自动填充 AI 的死亡（用于优先移除）
			if (AutoFillAIControllers.Contains(AICon))
			{
				RecentlyDeadAIControllers.AddUnique(AICon);
			}
		}

		// 检查是否需要该 AI 重生（玩家数量变化可能导致 AI 超额）
		const int32 NeededAI = CachedMaxPlayers - GetRealPlayerCount();
		const int32 CurrentAI = GetAutoFillAICount(); // 含存活+待重生
		if (bAutoFillWithAI && CurrentAI > NeededAI)
		{
			// AI 超额，不重生，直接清理
			if (AICon)
			{
				AutoFillAIControllers.Remove(AICon);
				RecentlyDeadAIControllers.Remove(AICon);
				ReturnAIName(AICon->GetPlayerState<APlayerState>() ?
					AICon->GetPlayerState<APlayerState>()->GetPlayerName() : TEXT(""));
			}
			// 延迟销毁 Pawn
			FTimerHandle DestroyHandle;
			FTimerDelegate DestroyDelegate;
			DestroyDelegate.BindLambda([Victim]() { if (IsValid(Victim)) Victim->Destroy(); });
			GetWorldTimerManager().SetTimer(DestroyHandle, DestroyDelegate, DestroyDelay, false);
			return;
		}

		ScheduleAIRespawn(VictimController, AIDelay);

		if (AFPSPlayerState* PS = VictimController ? VictimController->GetPlayerState<AFPSPlayerState>() : nullptr)
		{
			ReturnAIName(PS->GetPlayerName());
		}

		// 延迟销毁（让布娃娃可见）
		FTimerHandle DestroyHandle;
		FTimerDelegate DestroyDelegate;
		DestroyDelegate.BindLambda([Victim]()
		{
			if (IsValid(Victim)) Victim->Destroy();
		});
		GetWorldTimerManager().SetTimer(DestroyHandle, DestroyDelegate, DestroyDelay, false);
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
		GetWorldTimerManager().SetTimer(DestroyHandle, DestroyDelegate, DestroyDelay, false);

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

void AFPSGameMode::ScheduleAIRespawn(AController* AICon, float Delay)
{
	if (!AICon || !bGameActive) return;

	FTimerHandle& Handle = RespawnTimers.FindOrAdd(AICon);
	FTimerDelegate TimerDelegate;
	TimerDelegate.BindUObject(this, &AFPSGameMode::RespawnAI, AICon);
	GetWorldTimerManager().SetTimer(Handle, TimerDelegate, Delay, false);
}

void AFPSGameMode::RespawnAI(AController* AICon)
{
	if (!AICon || !bGameActive) return;

	RespawnTimers.Remove(AICon);

	// 从最近死亡列表中移除
	if (AFPS_AIController* AI = Cast<AFPS_AIController>(AICon))
	{
		RecentlyDeadAIControllers.Remove(AI);
	}

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

	// 清理自动填充 AI 列表
	AutoFillAIControllers.Empty();
	RecentlyDeadAIControllers.Empty();

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

// ======================================================================
// AI 自动填充
// ======================================================================

void AFPSGameMode::ReadMaxPlayersFromSession()
{
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (!OnlineSub) return;

	IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
	if (!Sessions.IsValid()) return;

	FNamedOnlineSession* Session = Sessions->GetNamedSession(NAME_GameSession);
	if (!Session) return;

	CachedMaxPlayers = Session->SessionSettings.NumPublicConnections;
	UE_LOG(LogTemp, Log, TEXT("ReadMaxPlayersFromSession: %d"), CachedMaxPlayers);
}

int32 AFPSGameMode::GetRealPlayerCount() const
{
	int32 Count = 0;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (PC && !Cast<AFPS_AIController>(PC))
		{
			Count++;
		}
	}
	return Count;
}

int32 AFPSGameMode::GetAutoFillAICount() const
{
	// 清理无效引用后计数
	int32 Count = 0;
	for (const auto& AICon : AutoFillAIControllers)
	{
		if (IsValid(AICon))
		{
			Count++;
		}
	}
	return Count;
}

void AFPSGameMode::FillAIIfNeeded()
{
	if (!bAutoFillWithAI || !AIFillCharacterClass || !AIFillControllerClass) return;

	const int32 RealPlayers = GetRealPlayerCount();
	const int32 CurrentAI = GetAutoFillAICount();
	const int32 NeededAI = CachedMaxPlayers - RealPlayers;

	UE_LOG(LogTemp, Log, TEXT("FillAIIfNeeded: RealPlayers=%d, CurrentAI=%d, Max=%d, Need=%d"),
		RealPlayers, CurrentAI, CachedMaxPlayers, NeededAI - CurrentAI);

	while (GetAutoFillAICount() < NeededAI)
	{
		AFPS_AIController* NewAI = SpawnOneAI();
		if (!NewAI)
		{
			UE_LOG(LogTemp, Error, TEXT("FillAIIfNeeded: Failed to spawn AI"));
			break;
		}
	}
}

void AFPSGameMode::RemoveAI(int32 Count)
{
	for (int32 i = 0; i < Count; i++)
	{
		// 优先移除最近死亡的 AI
		while (RecentlyDeadAIControllers.Num() > 0)
		{
			AFPS_AIController* DeadAI = RecentlyDeadAIControllers.Pop();
			if (IsValid(DeadAI) && AutoFillAIControllers.Contains(DeadAI))
			{
				// 取消重生定时器
				if (FTimerHandle* Handle = RespawnTimers.Find(DeadAI))
				{
					GetWorldTimerManager().ClearTimer(*Handle);
					RespawnTimers.Remove(DeadAI);
				}
				DestroyOneAI(DeadAI);
				Count--;
				break;
			}
		}

		if (Count <= 0) break;

		// 没有死亡的 AI，移除存活的
		while (AutoFillAIControllers.Num() > 0)
		{
			AFPS_AIController* AliveAI = AutoFillAIControllers.Pop();
			if (IsValid(AliveAI))
			{
				RecentlyDeadAIControllers.Remove(AliveAI);
				DestroyOneAI(AliveAI);
				break;
			}
		}
	}
}

AFPS_AIController* AFPSGameMode::SpawnOneAI()
{
	UWorld* World = GetWorld();
	if (!World) return nullptr;

	// 1. 随机选择出生点
	AActor* Start = FindPlayerStart_Implementation(nullptr);
	FVector SpawnLocation = Start ? Start->GetActorLocation() : FVector::ZeroVector;
	FRotator SpawnRotation = Start ? Start->GetActorRotation() : FRotator::ZeroRotator;

	// 2. 生成 AI 角色
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AFPS_AICharacter* AIChar = World->SpawnActor<AFPS_AICharacter>(
		AIFillCharacterClass, SpawnLocation, SpawnRotation, SpawnParams);
	if (!AIChar)
	{
		UE_LOG(LogTemp, Error, TEXT("SpawnOneAI: Failed to spawn AI character"));
		return nullptr;
	}

	// 3. 生成 AI 控制器
	AFPS_AIController* AICon = World->SpawnActor<AFPS_AIController>(
		AIFillControllerClass, FVector::ZeroVector, FRotator::ZeroRotator);
	if (!AICon)
	{
		UE_LOG(LogTemp, Error, TEXT("SpawnOneAI: Failed to spawn AI controller"));
		AIChar->Destroy();
		return nullptr;
	}

	// 4. 手动创建 PlayerState（AController 手动 SpawnActor 时引擎不会自动创建）
	//    真实玩家通过 GameMode::Login 流程自动创建，AI 需要手动处理
	AFPSPlayerState* PS = World->SpawnActor<AFPSPlayerState>(
		AFPSPlayerState::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
	if (PS)
	{
		AICon->SetPlayerState(PS);
		PS->SetOwner(AICon);
		PS->SetPlayerName(ClaimAIName());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnOneAI: Failed to create PlayerState for AI"));
	}

	// 5. Possess 角色（触发 OnPossess → 运行 BehaviorTree）
	AICon->Possess(AIChar);

	// 6. 绑定死亡委托
	BindCharacterDelegates(AIChar);

	// 7. 追踪
	AutoFillAIControllers.Add(AICon);

	UE_LOG(LogTemp, Log, TEXT("SpawnOneAI: Spawned AI '%s', Total=%d"),
		*GetControllerPlayerName(AICon), GetAutoFillAICount());

	return AICon;
}

void AFPSGameMode::DestroyOneAI(AFPS_AIController* AICon)
{
	if (!IsValid(AICon)) return;

	// 归还名称
	if (AFPSPlayerState* PS = AICon->GetPlayerState<AFPSPlayerState>())
	{
		ReturnAIName(PS->GetPlayerName());
		PS->Destroy();
	}

	// 销毁 Pawn
	if (APawn* Pawn = AICon->GetPawn())
	{
		Pawn->Destroy();
	}

	// 销毁控制器
	AICon->Destroy();

	// 从列表移除
	AutoFillAIControllers.Remove(AICon);
	RecentlyDeadAIControllers.Remove(AICon);

	UE_LOG(LogTemp, Log, TEXT("DestroyOneAI: Destroyed AI, Remaining=%d"), GetAutoFillAICount());
}
