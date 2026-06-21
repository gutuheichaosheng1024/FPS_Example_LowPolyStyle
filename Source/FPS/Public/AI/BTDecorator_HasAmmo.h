#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_HasAmmo.generated.h"

/**
 * UBTDecorator_HasAmmo — Behavior Tree decorator that checks if the AI's current weapon has ammo
 *
 * 职责：在行为树分支条件中检查AI角色的当前武器是否持有弹药，直接查询角色武器状态而非依赖黑板
 * 使用：AFPS_CharacterBase, AWeaponActor
 */
UCLASS()
class FPS_API UBTDecorator_HasAmmo : public UBTDecorator
{
    GENERATED_BODY()

public:
    UBTDecorator_HasAmmo();

protected:
    virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
};
