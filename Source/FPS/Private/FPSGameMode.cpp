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

// 构造函数：设置 GameMode 复制与 PlayerState 类
// 流程：bReplicates = true → 设置 PlayerStateClass = AFPSPlayerState
AFPSGameMode::AFPSGameMode()
{
	bReplicates = true;
	PlayerStateClass = AFPSPlayerState::StaticClass();
}

// GetLifetimeReplicatedProps：注册需要网络复制的 GameMode 属性
// 流程：调用 Super → 注册 RemainingTime → 注册 bGameActive
void AFPSGameMode::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AFPSGameMode, RemainingTime);
	DOREPLIFETIME(AFPSGameMode, bGameActive);
}

// BeginPlay：游戏开始，初始化 AI 名称池、读取 Session 配置、启动倒计时、补齐 AI
// 流程：初始化 AvailableAINames → ReadMaxPlayersFromSession → 设置 bGameActive / GameStartTime / RemainingTime →
//       启动 TickGameTimer (0.1s) → SyncGameTimeToAllPlayers → FillAIIfNeeded
void AFPSGameMode::BeginPlay()
{
	Super::BeginPlay();

	AvailableAINames = AINames;

	ReadMaxPlayersFromSession();

	bGameActive = true;
	GameStartTime = GetWorld()->GetTimeSeconds();
	RemainingTime = GameDuration;
	GetWorldTimerManager().SetTimer(GameTimerHandle, this, &AFPSGameMode::TickGameTimer, 0.1f, true);

	SyncGameTimeToAllPlayers();

	if (bAutoFillWithAI && AIFillCharacterClass && AIFillControllerClass)
	{
		FillAIIfNeeded();
	}
}

// RestartPlayer：引擎重生流程，重置 PlayerState 存活时间并绑定新角色委托
// 流程：Super::RestartPlayer → 重置 PlayerState 存活时间 → 同步游戏时间到新玩家 → 绑定死亡委托
void AFPSGameMode::RestartPlayer(AController* NewPlayer)
{
	Super::RestartPlayer(NewPlayer);

	if (AFPSPlayerState* PS = NewPlayer->GetPlayerState<AFPSPlayerState>())
	{
		PS->ResetSurvivalTime();

		SyncGameTimeToPlayer(PS);
	}

	if (APawn* NewPawn = NewPlayer->GetPawn())
	{
		BindCharacterDelegates(NewPawn);
	}
}

// FindPlayerStart_Implementation：随机选择 PlayerStart 出生点
// 流程：GetAllActorsOfClass(PlayerStart) → 随机选择一个索引 → 无 PlayerStart 则回退到 Super
AActor* AFPSGameMode::FindPlayerStart_Implementation(AController* Player, const FString& IncomingName)
{
	TArray<AActor*> AllPlayerStarts;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), AllPlayerStarts);

	if (AllPlayerStarts.Num() > 0)
	{
		int32 RandomIndex = FMath::RandRange(0, AllPlayerStarts.Num() - 1);
		return AllPlayerStarts[RandomIndex];
	}

	return Super::FindPlayerStart_Implementation(Player, IncomingName);
}

// OnPostLogin：玩家登录时从存档写入 PlayerName 到 PlayerState，并移除一个自动填充 AI
// 流程：Super::OnPostLogin → 读取存档 PlayerName 写入 PlayerState → 若 AI 自动填充启用则 RemoveAI(1)
void AFPSGameMode::OnPostLogin(AController* NewPlayer)
{
	Super::OnPostLogin(NewPlayer);

	if (!NewPlayer) return;

	if (APlayerState* PS = NewPlayer->GetPlayerState<APlayerState>())
	{
		const FString SavedName = UFPSSaveGame::LoadPlayerName();
		PS->SetPlayerName(SavedName);
	}

	if (bAutoFillWithAI)
	{
		RemoveAI(1);
	}
}

// Logout：玩家离开时延迟补充 AI（等待 GetNumPlayers 更新后再 FillAIIfNeeded）
// 流程：Super::Logout → 若 AI 自动填充启用则延迟 0.1s 调用 FillAIIfNeeded
void AFPSGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	if (bAutoFillWithAI)
	{
		FTimerHandle TempHandle;
		FTimerDelegate Del;
		Del.BindLambda([this]() { if (bGameActive) FillAIIfNeeded(); });
		GetWorldTimerManager().SetTimer(TempHandle, Del, 0.1f, false);
	}
}

// BindCharacterDelegates：绑定角色的 OnKilled 委托到 GameMode::OnCharacterKilled
// 流程：Cast AFPS_CharacterBase → AddDynamic
void AFPSGameMode::BindCharacterDelegates(APawn* Pawn)
{
	if (AFPS_CharacterBase* Char = Cast<AFPS_CharacterBase>(Pawn))
	{
		Char->OnKilled.AddDynamic(this, &AFPSGameMode::OnCharacterKilled);
	}
}

// TickGameTimer：游戏倒计时 Tick，更新 RemainingTime 并同步到所有 PlayerState
// 流程：检查 bGameActive → 计算 Elapsed = 当前时间 - GameStartTime → RemainingTime = max(GameDuration - Elapsed, 0) →
//       遍历所有 PlayerController 同步 RemainingTime → RemainingTime <= 0 则 EndGame
void AFPSGameMode::TickGameTimer()
{
	if (!bGameActive) return;

	const float Elapsed = GetWorld()->GetTimeSeconds() - GameStartTime;
	RemainingTime = FMath::Max(GameDuration - Elapsed, 0.f);

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

// OnCharacterKilled：角色死亡处理，分 AI/玩家两条分支处理重生/销毁/UI 推送
// 流程：校验 Victim 和 bGameActive → RegisterKill → 通知 KillerPC Client_OnKillConfirmed →
//       AI 分支：检查 AI 是否超额（超额则清理不重生）→ 否则 ScheduleAIRespawn → 分配新名称 → 延迟销毁 Pawn →
//       玩家分支：延迟销毁 Pawn → 推送 Client_ShowRespawnUI 带击杀者名称
void AFPSGameMode::OnCharacterKilled(AFPS_CharacterBase* Victim, AController* KillerController)
{
	if (!Victim || !bGameActive) return;

	AController* VictimController = Victim->GetController();

	RegisterKill(KillerController, VictimController);

	if (KillerController && KillerController != VictimController)
	{
		if (AFPSPlayerController* KillerPC = Cast<AFPSPlayerController>(KillerController))
		{
			KillerPC->Client_OnKillConfirmed();
		}
	}

	if (Cast<AFPS_AICharacter>(Victim))
	{
		AFPS_AIController* AICon = Cast<AFPS_AIController>(VictimController);
		float AIDelay = 5.f;
		if (AICon)
		{
			AIDelay = AICon->RespawnTime;

			if (AutoFillAIControllers.Contains(AICon))
			{
				RecentlyDeadAIControllers.AddUnique(AICon);
			}
		}

		const int32 NeededAI = CachedMaxPlayers - GetRealPlayerCount();
		const int32 CurrentAI = GetAutoFillAICount();
		if (bAutoFillWithAI && CurrentAI > NeededAI)
		{
			if (AICon)
			{
				AutoFillAIControllers.Remove(AICon);
				RecentlyDeadAIControllers.Remove(AICon);
				ReturnAIName(AICon->GetPlayerState<APlayerState>() ?
					AICon->GetPlayerState<APlayerState>()->GetPlayerName() : TEXT(""));
			}
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

// RegisterKill：击杀计分，更新受害者 Deaths 与击杀者 Kills/Score（滚雪球公式）
// 流程：确保受害者计分数据存在 → VictimData.Deaths++ → 同步 Victim PlayerState →
//       确保击杀者计分数据存在 → KillerData.Kills++ → 记录 LastKillerName →
//       Score = BaseKillScore + VictimSurvivalTime * SurvivalTimeWeight + KillerKills * KillCountWeight →
//       同步 Killer PlayerState
void AFPSGameMode::RegisterKill(AController* Killer, AController* Victim)
{
	if (!Victim) return;

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

	if (AFPSPlayerState* VictimPS = Victim->GetPlayerState<AFPSPlayerState>())
	{
		VictimPS->Deaths = VictimData.Deaths;
	}

	if (!Killer || Killer == Victim) return;

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

	VictimData.LastKillerName = KillerData.PlayerName;

	float VictimSurvivalTime = 0.f;
	if (AFPSPlayerState* VictimPS = Victim->GetPlayerState<AFPSPlayerState>())
	{
		VictimSurvivalTime = VictimPS->GetSurvivalTime();
	}

	float KillScore = BaseKillScore
		+ VictimSurvivalTime * SurvivalTimeWeight
		+ static_cast<float>(KillerData.Kills) * KillCountWeight;

	KillerData.TotalScore += KillScore;

	if (AFPSPlayerState* KillerPS = Killer->GetPlayerState<AFPSPlayerState>())
	{
		KillerPS->Kills = KillerData.Kills;
		KillerPS->SetScore(KillerData.TotalScore);
	}
}

// ScheduleAIRespawn：调度 AI 延迟重生定时器
// 流程：校验 AICon 和 bGameActive → 设置 RespawnTimers → 延迟 Delay 秒后调用 RespawnAI
void AFPSGameMode::ScheduleAIRespawn(AController* AICon, float Delay)
{
	if (!AICon || !bGameActive) return;

	FTimerHandle& Handle = RespawnTimers.FindOrAdd(AICon);
	FTimerDelegate TimerDelegate;
	TimerDelegate.BindUObject(this, &AFPSGameMode::RespawnAI, AICon);
	GetWorldTimerManager().SetTimer(Handle, TimerDelegate, Delay, false);
}

// RespawnAI：执行 AI 重生，清除死亡记录并标准重生
// 流程：清除 RespawnTimers → 移除 RecentlyDeadAIControllers → RestartPlayer → 分配新 AI 名称
void AFPSGameMode::RespawnAI(AController* AICon)
{
	if (!AICon || !bGameActive) return;

	RespawnTimers.Remove(AICon);

	if (AFPS_AIController* AI = Cast<AFPS_AIController>(AICon))
	{
		RecentlyDeadAIControllers.Remove(AI);
	}

	RestartPlayer(AICon);

	if (AFPSPlayerState* PS = AICon->GetPlayerState<AFPSPlayerState>())
	{
		PS->SetPlayerName(ClaimAIName());
	}
}

// EndGame：游戏结束，停止计时、清除重生定时器、排序计分板、推送 Client_GameEnd
// 流程：bGameActive = false → 清除 GameTimerHandle → 清除所有 RespawnTimers → 清理 AI 列表 →
//       Scoreboard 降序排序 → 遍历所有 FPSPlayerController 推送前三名数据
void AFPSGameMode::EndGame()
{
	bGameActive = false;
	GetWorldTimerManager().ClearTimer(GameTimerHandle);

	for (auto& Pair : RespawnTimers)
	{
		GetWorldTimerManager().ClearTimer(Pair.Value);
	}
	RespawnTimers.Empty();

	AutoFillAIControllers.Empty();
	RecentlyDeadAIControllers.Empty();

	Scoreboard.Sort([](const FPlayerScoreData& A, const FPlayerScoreData& B) {
		return A.TotalScore > B.TotalScore;
	});

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

// SyncGameTimeToAllPlayers：将 GameStartTime 和 GameDuration 同步到所有已连接的 PlayerState
// 流程：遍历所有 PlayerController → 调用 SyncGameTimeToPlayer
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

// SyncGameTimeToPlayer：将游戏时间戳写入单个 PlayerState
// 流程：检查 PS 有效性 → PS->GameStartTime = GameStartTime → PS->GameDuration = GameDuration
void AFPSGameMode::SyncGameTimeToPlayer(AFPSPlayerState* PS)
{
	if (!PS) return;
	PS->GameStartTime = GameStartTime;
	PS->GameDuration = GameDuration;
}

// GetTopPlayers：获取计分板前 N 名玩家数据
// 流程：复制 Scoreboard → 降序排序 → 截取前 Count 名
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

// ClaimAIName：从可用名称池中随机获取一个 AI 名称
// 流程：检查 AvailableAINames → 非空则随机取一个并移除 → 池空则用 FallbackIndex 生成 Bot_N
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

// ReturnAIName：将 AI 名称归还到可用池
// 流程：检查名称不在池中且属于 AINames 预设 → 添加回 AvailableAINames
void AFPSGameMode::ReturnAIName(const FString& Name)
{
	if (!AvailableAINames.Contains(Name) && AINames.Contains(Name))
	{
		AvailableAINames.Add(Name);
	}
}

// GetControllerPlayerName：从 Controller 获取玩家名称
// 流程：检查 Ctrl 和 PlayerState → 返回 PlayerName → 空则返回 "Unknown"
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

// ReadMaxPlayersFromSession：从 OnlineSession 读取最大玩家数
// 流程：获取 IOnlineSubsystem → 获取 SessionInterface → 获取 NamedSession(NAME_GameSession) → 读取 NumPublicConnections
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

// GetRealPlayerCount：统计当前真实玩家数量（排除 AI 控制器）
// 流程：遍历 PlayerControllerIterator → 排除 AFPS_AIController → 计数
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

// GetAutoFillAICount：统计当前存活的自动填充 AI 数量
// 流程：遍历 AutoFillAIControllers → 检查 IsValid → 计数
int32 AFPSGameMode::GetAutoFillAICount() const
{
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

// FillAIIfNeeded：按需补充 AI 到 Session 最大人数
// 流程：计算 RealPlayers 和 NeededAI → while 循环 SpawnOneAI 直到达到目标数量
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

// RemoveAI：移除指定数量的自动填充 AI，优先移除最近死亡的
// 流程：循环 Count 次 → 优先 Pop RecentlyDeadAIControllers 并清理定时器/销毁 →
//       死亡队列空则 Pop AutoFillAIControllers（存活）并销毁
void AFPSGameMode::RemoveAI(int32 Count)
{
	for (int32 i = 0; i < Count; i++)
	{
		while (RecentlyDeadAIControllers.Num() > 0)
		{
			AFPS_AIController* DeadAI = RecentlyDeadAIControllers.Pop();
			if (IsValid(DeadAI) && AutoFillAIControllers.Contains(DeadAI))
			{
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

// SpawnOneAI：生成一个自动填充 AI（角色 + 控制器 + PlayerState + Possess）
// 流程：随机选 PlayerStart → Spawn AICharacter → Spawn AIController → 手动创建 PlayerState 并 SetPlayerState →
//       Possess 角色 → 绑定死亡委托 → 加入 AutoFillAIControllers 列表
AFPS_AIController* AFPSGameMode::SpawnOneAI()
{
	UWorld* World = GetWorld();
	if (!World) return nullptr;

	AActor* Start = FindPlayerStart_Implementation(nullptr);
	FVector SpawnLocation = Start ? Start->GetActorLocation() : FVector::ZeroVector;
	FRotator SpawnRotation = Start ? Start->GetActorRotation() : FRotator::ZeroRotator;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AFPS_AICharacter* AIChar = World->SpawnActor<AFPS_AICharacter>(
		AIFillCharacterClass, SpawnLocation, SpawnRotation, SpawnParams);
	if (!AIChar)
	{
		UE_LOG(LogTemp, Error, TEXT("SpawnOneAI: Failed to spawn AI character"));
		return nullptr;
	}

	AFPS_AIController* AICon = World->SpawnActor<AFPS_AIController>(
		AIFillControllerClass, FVector::ZeroVector, FRotator::ZeroRotator);
	if (!AICon)
	{
		UE_LOG(LogTemp, Error, TEXT("SpawnOneAI: Failed to spawn AI controller"));
		AIChar->Destroy();
		return nullptr;
	}

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

	AICon->Possess(AIChar);

	BindCharacterDelegates(AIChar);

	AutoFillAIControllers.Add(AICon);

	UE_LOG(LogTemp, Log, TEXT("SpawnOneAI: Spawned AI '%s', Total=%d"),
		*GetControllerPlayerName(AICon), GetAutoFillAICount());

	return AICon;
}

// DestroyOneAI：销毁一个自动填充 AI（归还名称、销毁 Pawn 和 Controller）
// 流程：归还 AI 名称到池 → 销毁 PlayerState → 销毁 Pawn → 销毁 Controller → 从追踪列表移除
void AFPSGameMode::DestroyOneAI(AFPS_AIController* AICon)
{
	if (!IsValid(AICon)) return;

	if (AFPSPlayerState* PS = AICon->GetPlayerState<AFPSPlayerState>())
	{
		ReturnAIName(PS->GetPlayerName());
		PS->Destroy();
	}

	if (APawn* Pawn = AICon->GetPawn())
	{
		Pawn->Destroy();
	}

	AICon->Destroy();

	AutoFillAIControllers.Remove(AICon);
	RecentlyDeadAIControllers.Remove(AICon);

	UE_LOG(LogTemp, Log, TEXT("DestroyOneAI: Destroyed AI, Remaining=%d"), GetAutoFillAICount());
}
