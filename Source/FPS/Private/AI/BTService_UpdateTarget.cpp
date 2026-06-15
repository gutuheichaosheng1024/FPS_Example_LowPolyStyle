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

void UBTService_UpdateTarget::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    AFPS_AICharacter* AIChar = Cast<AFPS_AICharacter>(OwnerComp.GetAIOwner()->GetPawn());
    if (!AIChar) return;

    UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
    if (!BB) return;

    AActor* Target = Cast<AActor>(BB->GetValueAsObject(AFPS_AIController::BBKey_TargetActor));
    if (Target)
    {
        // 目标已死亡 → 清除 TargetActor，BT 自动回 Patrol
        if (const AFPS_CharacterBase* TargetChar = Cast<AFPS_CharacterBase>(Target))
        {
            if (TargetChar->IsDead())
            {
                BB->ClearValue(AFPS_AIController::BBKey_TargetActor);
                return;
            }
        }

        // 持续刷新目标位置（供 MoveTo 使用）
        BB->SetValueAsVector(AFPS_AIController::BBKey_LastKnownLocation, Target->GetActorLocation());

        // 更新瞄准方向
        AIChar->AimAtTarget(Target);

        // 距离
        const float Distance = FVector::Dist(AIChar->GetActorLocation(), Target->GetActorLocation());
        BB->SetValueAsFloat(AFPS_AIController::BBKey_DistanceToTarget, Distance);

        // 距离 + 视线 → 合并判定 ShouldEngage
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
