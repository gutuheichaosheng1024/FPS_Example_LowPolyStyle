#pragma once

#include "CoreMinimal.h"
#include "Character/FPS_CharacterBase.h"
#include "GenericTeamAgentInterface.h"
#include "FPS_AICharacter.generated.h"

class UAIAttackConfig;

UCLASS(config = Game)
class FPS_API AFPS_AICharacter : public AFPS_CharacterBase, public IGenericTeamAgentInterface
{
    GENERATED_BODY()

public:
    AFPS_AICharacter();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // --- IGenericTeamAgentInterface ---
    virtual FGenericTeamId GetGenericTeamId() const override { return TeamId; }
    virtual void SetGenericTeamId(const FGenericTeamId& NewTeamId) override { TeamId = NewTeamId; }

    // --- 多态接口覆盖 ---
    virtual FVector GetShootLocation() const override;
    virtual FRotator GetShootRotation() const override;

    // --- AI 专用接口 ---
    UFUNCTION(BlueprintCallable, Category = "AI|Combat")
    void FireAtTarget(AActor* Target);

    UFUNCTION(BlueprintCallable, Category = "AI|Combat")
    void StopFiring();

    UFUNCTION(BlueprintCallable, Category = "AI|Combat")
    void ReloadWeapon();

    UFUNCTION(BlueprintCallable, Category = "AI|Combat")
    void AimAtTarget(AActor* Target);

    virtual void HandleDeath() override;
    virtual void Multicast_OnDeath_Implementation() override;

    /** 瞄准目标位置 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "AI|Combat")
    FVector AimTargetLocation;

    /** 当前攻击目标 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Combat")
    TWeakObjectPtr<AActor> CurrentTarget;

    /** AI 攻击配置（命中概率/骨骼选择/距离阈值），BT 节点通过此访问 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat")
    TObjectPtr<UAIAttackConfig> AttackConfig;

protected:
    void UpdateAimDirection(float DeltaTime);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
    FGenericTeamId TeamId;

    /** 射击起点的骨骼/插槽名，留空则用 ActorLocation */
    UPROPERTY(EditAnywhere, Category = "AI|Combat")
    FName ShootBoneName = NAME_None;

    UPROPERTY(EditAnywhere, Category = "AI|Combat",
        meta = (ClampMin = "0.5", ClampMax = "20.0"))
    float AimRotationSpeed = 10.f;

    bool bIsFiring = false;
};
