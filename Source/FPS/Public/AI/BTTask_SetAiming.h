#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_SetAiming.generated.h"

/**
 * UBTTask_SetAiming — Behavior Tree task that toggles AI aiming state on/off
 *
 * 职责：在行为树中设置AI角色的瞄准状态，执行时根据bAim标志调用Aim()或StopAim()，中止时强制StopAim()
 * 使用：AFPS_CharacterBase
 */
UCLASS()
class FPS_API UBTTask_SetAiming : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_SetAiming();

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bAim = true;

protected:
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
    virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
