#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_UpdateTarget.generated.h"

/**
 * UBTService_UpdateTarget — Behavior Tree service that periodically refreshes target info on the Blackboard
 *
 * 职责：以0.15s间隔更新AI目标相关黑板数据：目标存活检查、位置刷新、距离计算、视线检测、ShouldEngage判定
 * 使用：AFPS_AICharacter, AFPS_CharacterBase, AFPS_AIController, UAIAttackConfig, UBlackboardComponent
 */
UCLASS()
class FPS_API UBTService_UpdateTarget : public UBTService
{
    GENERATED_BODY()

public:
    UBTService_UpdateTarget();

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FBlackboardKeySelector TargetActorKey;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FBlackboardKeySelector HasLineOfSightKey;

protected:
    virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};
