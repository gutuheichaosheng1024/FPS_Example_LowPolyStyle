#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_FindRandomPatrolPoint.generated.h"

UCLASS()
class FPS_API UBTTask_FindRandomPatrolPoint : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_FindRandomPatrolPoint();

    /** 搜索半径 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI",
        meta = (ClampMin = "100.0", ClampMax = "10000.0"))
    float SearchRadius = 2000.f;

    /** 结果写入的黑板 Key */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FBlackboardKeySelector OutputLocationKey;

protected:
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
