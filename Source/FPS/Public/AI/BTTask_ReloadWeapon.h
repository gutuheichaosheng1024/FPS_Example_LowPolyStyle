#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_ReloadWeapon.generated.h"

/**
 * UBTTask_ReloadWeapon — Behavior Tree task that triggers AI weapon reload
 *
 * 职责：在行为树中触发AI角色执行武器换弹操作，调用AICharacter的ReloadWeapon接口
 * 使用：AFPS_AICharacter
 */
UCLASS()
class FPS_API UBTTask_ReloadWeapon : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_ReloadWeapon();

protected:
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
