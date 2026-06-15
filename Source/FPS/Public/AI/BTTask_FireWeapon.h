#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_FireWeapon.generated.h"

class UAIAttackConfig;

struct FFireWeaponTaskMemory
{
    float TimeSinceLastShot = 0.f;
};

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

    /** 根据命中概率更新 AimTargetLocation */
    void ApplyAimOffset(class AFPS_AICharacter* AIChar, const AActor* Target) const;
};
