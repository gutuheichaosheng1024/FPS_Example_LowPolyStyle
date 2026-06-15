#include "AI/BTTask_FaceTarget.h"
#include "Character/FPS_AICharacter.h"
#include "AI/FPS_AIController.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_FaceTarget::UBTTask_FaceTarget()
{
    NodeName = TEXT("Face Target");
    bNotifyTick = true;
}

EBTNodeResult::Type UBTTask_FaceTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AFPS_AICharacter* AIChar = Cast<AFPS_AICharacter>(OwnerComp.GetAIOwner()->GetPawn());
    UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
    if (!AIChar || !BB) return EBTNodeResult::Failed;

    AActor* Target = Cast<AActor>(BB->GetValueAsObject(AFPS_AIController::BBKey_TargetActor));
    if (!Target) return EBTNodeResult::Failed;

    AIChar->AimTargetLocation = Target->GetActorLocation();
    return EBTNodeResult::InProgress;
}

void UBTTask_FaceTarget::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    AFPS_AICharacter* AIChar = Cast<AFPS_AICharacter>(OwnerComp.GetAIOwner()->GetPawn());
    UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
    if (!AIChar || !BB) { FinishLatentTask(OwnerComp, EBTNodeResult::Failed); return; }

    AActor* Target = Cast<AActor>(BB->GetValueAsObject(AFPS_AIController::BBKey_TargetActor));
    if (!Target) { FinishLatentTask(OwnerComp, EBTNodeResult::Failed); return; }

    // 持续更新目标位置（目标可能移动）
    AIChar->AimTargetLocation = Target->GetActorLocation();

    // 计算水平夹角
    const FVector DirToTarget = (Target->GetActorLocation() - AIChar->GetActorLocation()).GetSafeNormal2D();
    const FVector AIDir = AIChar->GetActorForwardVector().GetSafeNormal2D();
    const float Dot = FVector::DotProduct(AIDir, DirToTarget);
    const float Angle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Dot, -1.f, 1.f)));

    if (Angle <= AcceptableAngle)
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
    }
}
