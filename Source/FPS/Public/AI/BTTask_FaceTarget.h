#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_FaceTarget.generated.h"

UCLASS()
class FPS_API UBTTask_FaceTarget : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_FaceTarget();

    /** 允许的角度偏差（度），AI 面向目标角度小于此值即完成 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI",
        meta = (ClampMin = "1.0", ClampMax = "90.0"))
    float AcceptableAngle = 15.0f;

protected:
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
    virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
    virtual uint16 GetInstanceMemorySize() const override { return 0; }
};
