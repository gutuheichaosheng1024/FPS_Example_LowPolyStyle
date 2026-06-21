#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_SetSprinting.generated.h"

/**
 * UBTTask_SetSprinting — Behavior Tree task that toggles AI sprinting state on/off
 *
 * 职责：在行为树中设置AI角色的冲刺状态，执行时根据bSprint标志调用Sprint()或StopSprint()，中止时强制StopSprint()
 * 使用：AFPS_CharacterBase
 */
UCLASS()
class FPS_API UBTTask_SetSprinting : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_SetSprinting();

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bSprint = true;

protected:
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
    virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
