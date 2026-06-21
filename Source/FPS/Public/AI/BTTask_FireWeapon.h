#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_FireWeapon.generated.h"

class UAIAttackConfig;

struct FFireWeaponTaskMemory
{
    float TimeSinceLastShot = 0.f;
};

/**
 * UBTTask_FireWeapon — Behavior Tree task that controls AI weapon firing with aim offset and fire mode support
 *
 * 职责：驱动AI角色持续开火，每帧根据命中概率更新瞄准偏移，支持半自动/全自动射击模式，管理弹药耗尽和丢失目标时的中止
 * 使用：AFPS_AICharacter, AFPS_AIController, UAIAttackConfig, AWeaponActor, UWeaponDataConfig
 */
UCLASS()
class FPS_API UBTTask_FireWeapon : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_FireWeapon();

protected:
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
    virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
    virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
    virtual uint16 GetInstanceMemorySize() const override { return sizeof(FFireWeaponTaskMemory); }

    void ApplyAimOffset(class AFPS_AICharacter* AIChar, const AActor* Target) const;
};
