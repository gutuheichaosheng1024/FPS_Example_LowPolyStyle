#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "FPSGameMode.generated.h"

class AFPS_CharacterBase;
class AFPS_AICharacter;
class AFPS_AIController;
class AFPSPlayerState;

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

	FString LastKillerName;
};

/**
 * AFPSGameMode — 游戏内 GameMode，管理计时/计分/重生/AI/游戏结束全生命周期
 *
 * 职责：GameDuration 倒计时与 RemainingTime 同步、滚雪球击杀计分与计分板维护、玩家/AI 重启与延迟销毁、
 *       AI 自动填充（按 Session 最大人数补齐/移除）、AI 名称池管理、游戏结束时排序并推送结算 UI
 * 使用：AFPS_CharacterBase, AFPS_AICharacter, AFPS_AIController, AFPSPlayerState, AFPSPlayerController
 */
UCLASS()
class FPS_API AFPSGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AFPSGameMode();
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void RestartPlayer(AController* NewPlayer) override;
	virtual void OnPostLogin(AController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

	virtual AActor* FindPlayerStart_Implementation(AController* Player, const FString& IncomingName = TEXT("")) override;

	UFUNCTION()
	void OnCharacterKilled(AFPS_CharacterBase* Victim, AController* KillerController);

	UFUNCTION(BlueprintPure, Category = "Score")
	TArray<FPlayerScoreData> GetTopPlayers(int32 Count = 3) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game")
	float GameDuration = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoring")
	float BaseKillScore = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoring")
	float SurvivalTimeWeight = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoring")
	float KillCountWeight = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	TArray<FString> AINames;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|AutoFill")
	bool bAutoFillWithAI = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|AutoFill",
		meta = (EditCondition = "bAutoFillWithAI"))
	TSubclassOf<AFPS_AICharacter> AIFillCharacterClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|AutoFill",
		meta = (EditCondition = "bAutoFillWithAI"))
	TSubclassOf<AFPS_AIController> AIFillControllerClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Respawn")
	float DestroyDelay = 3.f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game")
	float RemainingTime = 0.f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game")
	bool bGameActive = false;

protected:
	void TickGameTimer();
	void SyncGameTimeToAllPlayers();
	void SyncGameTimeToPlayer(AFPSPlayerState* PS);
	void RegisterKill(AController* Killer, AController* Victim);
	void EndGame();
	void ScheduleAIRespawn(AController* AICon, float Delay);
	void RespawnAI(AController* AICon);
	void BindCharacterDelegates(APawn* Pawn);

	void ReadMaxPlayersFromSession();
	int32 GetRealPlayerCount() const;
	int32 GetAutoFillAICount() const;
	void FillAIIfNeeded();
	void RemoveAI(int32 Count);
	AFPS_AIController* SpawnOneAI();
	void DestroyOneAI(AFPS_AIController* AICon);

	FString GetControllerPlayerName(AController* Ctrl) const;
	FString ClaimAIName();
	void ReturnAIName(const FString& Name);

	float GameStartTime = 0.f;
	TArray<FPlayerScoreData> Scoreboard;
	TMap<AController*, int32> ControllerScoreIndex;
	TArray<FString> AvailableAINames;
	FTimerHandle GameTimerHandle;
	TMap<AController*, FTimerHandle> RespawnTimers;

	int32 CachedMaxPlayers = 4;
	TArray<TObjectPtr<AFPS_AIController>> AutoFillAIControllers;
	TArray<TObjectPtr<AFPS_AIController>> RecentlyDeadAIControllers;
};
