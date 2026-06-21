#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_ClearBlackboardKey.generated.h"

/**
 * UBTTask_ClearBlackboardKey — Behavior Tree task that clears a specified Blackboard key
 *
 * 职责：在行为树中清除指定的黑板键值，用于重置状态标志或清理目标引用
 * 使用：UBlackboardComponent
 */
UCLASS()
class FPS_API UBTTask_ClearBlackboardKey : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_ClearBlackboardKey();

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FBlackboardKeySelector KeyToClear;

protected:
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
