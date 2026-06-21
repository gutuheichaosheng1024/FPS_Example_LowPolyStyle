#pragma once

#include "CoreMinimal.h"
#include "Character/FPS_CharacterBase.h"
#include "GenericTeamAgentInterface.h"
#include "FPS_AICharacter.generated.h"

class UAIAttackConfig;

/**
 * AFPS_AICharacter — AI-controlled enemy character driven by behavior tree
 *
 * 职责：行为树驱动的射击/换弹/瞄准接口、瞄准方向插值旋转、队伍标识与敌我判断、死亡时停止行为树与布娃娃表现
 * 使用：AWeaponActor, UAIAttackConfig, AAIController, AFPS_CharacterBase
 */
UCLASS(config = Game)
class FPS_API AFPS_AICharacter : public AFPS_CharacterBase, public IGenericTeamAgentInterface
{
    GENERATED_BODY()

public:
    AFPS_AICharacter();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    virtual FGenericTeamId GetGenericTeamId() const override { return TeamId; }
    virtual void SetGenericTeamId(const FGenericTeamId& NewTeamId) override { TeamId = NewTeamId; }

    virtual FVector GetShootLocation() const override;
    virtual FRotator GetShootRotation() const override;

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

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "AI|Combat")
    FVector AimTargetLocation;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Combat")
    TWeakObjectPtr<AActor> CurrentTarget;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat")
    TObjectPtr<UAIAttackConfig> AttackConfig;

protected:
    void UpdateAimDirection(float DeltaTime);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
    FGenericTeamId TeamId;

    UPROPERTY(EditAnywhere, Category = "AI|Combat")
    FName ShootBoneName = NAME_None;

    UPROPERTY(EditAnywhere, Category = "AI|Combat",
        meta = (ClampMin = "0.5", ClampMax = "20.0"))
    float AimRotationSpeed = 10.f;

    bool bIsFiring = false;
};
