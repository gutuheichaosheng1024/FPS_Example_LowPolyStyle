#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_FaceTarget.generated.h"

/**
 * UBTTask_FaceTarget — Behavior Tree task that rotates AI to face its current target
 *
 * 职责：在行为树中使AI角色持续转向面向当前目标，每Tick更新目标位置并计算水平夹角，当角度偏差小于AcceptableAngle时完成
 * 使用：AFPS_AICharacter, AFPS_AIController, UBlackboardComponent
 */
UCLASS()
class FPS_API UBTTask_FaceTarget : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_FaceTarget();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI",
        meta = (ClampMin = "1.0", ClampMax = "90.0"))
    float AcceptableAngle = 15.0f;

protected:
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
    virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
    virtual uint16 GetInstanceMemorySize() const override { return 0; }
};
