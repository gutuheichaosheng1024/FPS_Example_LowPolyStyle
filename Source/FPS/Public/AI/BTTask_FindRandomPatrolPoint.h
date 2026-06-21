#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_FindRandomPatrolPoint.generated.h"

/**
 * UBTTask_FindRandomPatrolPoint — Behavior Tree task that finds a random reachable NavMesh point for patrol
 *
 * 职责：在AI巡逻行为中，以AI当前位置为中心在SearchRadius范围内通过导航系统随机查找可达点，结果写入指定的黑板Key
 * 使用：UNavigationSystemV1, UBlackboardComponent
 */
UCLASS()
class FPS_API UBTTask_FindRandomPatrolPoint : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_FindRandomPatrolPoint();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI",
        meta = (ClampMin = "100.0", ClampMax = "10000.0"))
    float SearchRadius = 2000.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FBlackboardKeySelector OutputLocationKey;

protected:
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
