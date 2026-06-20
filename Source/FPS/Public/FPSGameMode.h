#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "FPSGameMode.generated.h"

class AFPS_CharacterBase;
class AFPS_AICharacter;
class AFPS_AIController;
class AFPSPlayerState;

/** 计分板条目（服务器内存） */
USTRUCT(BlueprintType)
struct FPS_API FPlayerScoreData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FString PlayerName;

	UPROPERTY(BlueprintReadOnly)
	float TotalScore = 0.f;

	UPROPERTY(BlueprintReadOnly)
	int32 Kills = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 Deaths = 0;

	/** 最近一次被击杀时的击杀者名称（供重生UI显示） */
	FString LastKillerName;
};

/**
 * 游戏内 GameMode（与主菜单地图无关）
 *
 * 职责：
 * - 计时：GameDuration 倒计时，Replicated RemainingTime
 * - 计分：RegisterKill 更新计分板 + PlayerState
 * - 重生：RestartPlayer（销毁旧角色+生成新角色）
 * - AI管理：死亡后延迟重生 + AI名称池
 * - 游戏结束：排序计分板 → 逐客户端推送 Client_GameEnd
 */
UCLASS()
class FPS_API AFPSGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AFPSGameMode();
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ---------- 引擎覆盖 ----------

	virtual void RestartPlayer(AController* NewPlayer) override;
	virtual void OnPostLogin(AController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

	/** 随机选择 PlayerStart（覆盖默认选择逻辑） */
	virtual AActor* FindPlayerStart_Implementation(AController* Player, const FString& IncomingName = TEXT("")) override;

	// ---------- 计分 ----------

	/** 登记一次击杀（被击杀者的HandleDeath通过OnKilled委托触发） */
	UFUNCTION()
	void OnCharacterKilled(AFPS_CharacterBase* Victim, AController* KillerController);

	/** 获取前三名数据 */
	UFUNCTION(BlueprintPure, Category = "Score")
	TArray<FPlayerScoreData> GetTopPlayers(int32 Count = 3) const;

	// ---------- 配置属性 ----------

	/** 游戏总时长（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game")
	float GameDuration = 300.f;

	/** 基础击杀分 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoring")
	float BaseKillScore = 100.f;

	/** 被击杀者存活时间权重 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoring")
	float SurvivalTimeWeight = 0.5f;

	/** 击杀者总击杀数权重（滚雪球机制） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoring")
	float KillCountWeight = 20.f;

	/** AI 随机名称预设 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	TArray<FString> AINames;

	// ---------- AI 自动填充 ----------

	/** 是否启用 AI 自动填充（当玩家不足时用 AI 补齐到房间最大人数） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|AutoFill")
	bool bAutoFillWithAI = false;

	/** AI 角色类（蓝图中设置为 AFPS_AICharacter 的子类） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|AutoFill",
		meta = (EditCondition = "bAutoFillWithAI"))
	TSubclassOf<AFPS_AICharacter> AIFillCharacterClass;

	/** AI 控制器类（蓝图中设置为 AFPS_AIController 的子类） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|AutoFill",
		meta = (EditCondition = "bAutoFillWithAI"))
	TSubclassOf<AFPS_AIController> AIFillControllerClass;

	/** 角色死亡后销毁延迟（秒，给布娃娃保留时间） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Respawn")
	float DestroyDelay = 3.f;

	// ---------- 运行时状态 ----------

	/** 剩余时间（Replicated，客户端UI可读） */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game")
	float RemainingTime = 0.f;

	/** 游戏是否进行中 */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game")
	bool bGameActive = false;

protected:
	// ---------- 内部方法 ----------

	/** 游戏计时器 Tick（每 0.1 秒） */
	void TickGameTimer();

	/** 同步 GameStartTime/GameDuration 到所有 PlayerState */
	void SyncGameTimeToAllPlayers();

	/** 同步 GameStartTime/GameDuration 到单个 PlayerState */
	void SyncGameTimeToPlayer(AFPSPlayerState* PS);

	/** 执行击杀计分 */
	void RegisterKill(AController* Killer, AController* Victim);

	/** 游戏结束：排序计分板+推送Client_GameEnd */
	void EndGame();

	/** 调度AI延迟重生（延迟时间从 AIController 读取） */
	void ScheduleAIRespawn(AController* AICon, float Delay);

	/** 执行AI重生 */
	void RespawnAI(AController* AICon);

	/** 绑定新角色的死亡委托 */
	void BindCharacterDelegates(APawn* Pawn);

	// ---------- AI 自动填充内部方法 ----------

	/** 从 Session 读取最大玩家数 */
	void ReadMaxPlayersFromSession();

	/** 获取当前真实玩家数量（不含 AI） */
	int32 GetRealPlayerCount() const;

	/** 获取当前自动填充 AI 数量 */
	int32 GetAutoFillAICount() const;

	/** 补充 AI 到房间最大人数 */
	void FillAIIfNeeded();

	/** 移除指定数量的 AI（优先移除最近死亡的） */
	void RemoveAI(int32 Count);

	/** 生成一个自动填充 AI */
	AFPS_AIController* SpawnOneAI();

	/** 销毁一个 AI 控制器及其 Pawn */
	void DestroyOneAI(AFPS_AIController* AICon);

	/** 从控制器获取玩家名 */
	FString GetControllerPlayerName(AController* Ctrl) const;

	/** 分配/归还 AI 名称 */
	FString ClaimAIName();
	void ReturnAIName(const FString& Name);

	// ---------- 服务器内存状态 ----------

	/** 游戏开始时的服务器时间戳 */
	float GameStartTime = 0.f;

	/** 计分板 */
	TArray<FPlayerScoreData> Scoreboard;

	/** Controller → Scoreboard 索引快速查找 */
	TMap<AController*, int32> ControllerScoreIndex;

	/** AI 名称可用池 */
	TArray<FString> AvailableAINames;

	/** 计时器句柄 */
	FTimerHandle GameTimerHandle;

	/** 重生定时器（按Controller） */
	TMap<AController*, FTimerHandle> RespawnTimers;

	// ---------- AI 自动填充运行时状态 ----------

	/** 服务器记录的最大玩家数（从 Session 读取） */
	int32 CachedMaxPlayers = 4;

	/** 已生成的自动填充 AI 控制器列表 */
	TArray<TObjectPtr<AFPS_AIController>> AutoFillAIControllers;

	/** 最近死亡的 AI 控制器（用于优先移除） */
	TArray<TObjectPtr<AFPS_AIController>> RecentlyDeadAIControllers;
};
