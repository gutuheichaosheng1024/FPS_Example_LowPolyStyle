#include "AI/BTService_UpdateTarget.h"
#include "Character/FPS_AICharacter.h"
#include "Character/FPS_CharacterBase.h"
#include "AI/FPS_AIController.h"
#include "Weapon/AIAttackConfig.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTService_UpdateTarget::UBTService_UpdateTarget()
{
    NodeName = TEXT("Update Target");
    Interval = 0.15f;
}

// 定期更新目标信息：死亡检测 → 位置刷新 → 瞄准更新 → 距离计算 → 视线检测 → ShouldEngage联合判定
// 流程：验证AIChar/BB/Target → 目标死亡则清除TargetActor → 刷新LastKnownLocation → AimAtTarget → 计算Distance并更新InFireRange → LineOfSightTo → 合并bInRange&&bHasLOS写入ShouldEngage
void UBTService_UpdateTarget::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    AFPS_AICharacter* AIChar = Cast<AFPS_AICharacter>(OwnerComp.GetAIOwner()->GetPawn());
    if (!AIChar) return;

    UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
    if (!BB) return;

    AActor* Target = Cast<AActor>(BB->GetValueAsObject(AFPS_AIController::BBKey_TargetActor));
    if (Target)
    {
        if (const AFPS_CharacterBase* TargetChar = Cast<AFPS_CharacterBase>(Target))
        {
            if (TargetChar->IsDead())
            {
                BB->ClearValue(AFPS_AIController::BBKey_TargetActor);
                return;
            }
        }

        BB->SetValueAsVector(AFPS_AIController::BBKey_LastKnownLocation, Target->GetActorLocation());

        AIChar->AimAtTarget(Target);

        const float Distance = FVector::Dist(AIChar->GetActorLocation(), Target->GetActorLocation());
        BB->SetValueAsFloat(AFPS_AIController::BBKey_DistanceToTarget, Distance);

        const UAIAttackConfig* Config = AIChar->AttackConfig;
        const float FireRange = Config ? Config->FireRange : 1500.f;
        const bool bInRange = (Distance <= FireRange);

        BB->SetValueAsBool(AFPS_AIController::BBKey_InFireRange, bInRange);

        AAIController* AICon = OwnerComp.GetAIOwner();
        bool bHasLOS = false;
        if (AICon)
        {
            bHasLOS = AICon->LineOfSightTo(Target);
            BB->SetValueAsBool(AFPS_AIController::BBKey_HasLineOfSight, bHasLOS);
        }

        BB->SetValueAsBool(AFPS_AIController::BBKey_ShouldEngage, bInRange && bHasLOS);
    }
}
