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

// 开始面向目标：验证AI角色和目标 → 设置AimTargetLocation → 进入InProgress持续Tick
// 流程：获取AICharacter和BB → 从黑板读取TargetActor → 设置AimTargetLocation为目标位置 → 返回InProgress
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

// 每Tick更新面向：持续更新目标位置 → 计算水平夹角 → 角度达标则完成
// 流程：验证AIChar/BB/Target → 更新AimTargetLocation → 计算DirToTarget与AIDir的水平夹角 → 若角度≤AcceptableAngle则Succeeded
void UBTTask_FaceTarget::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    AFPS_AICharacter* AIChar = Cast<AFPS_AICharacter>(OwnerComp.GetAIOwner()->GetPawn());
    UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
    if (!AIChar || !BB) { FinishLatentTask(OwnerComp, EBTNodeResult::Failed); return; }

    AActor* Target = Cast<AActor>(BB->GetValueAsObject(AFPS_AIController::BBKey_TargetActor));
    if (!Target) { FinishLatentTask(OwnerComp, EBTNodeResult::Failed); return; }

    AIChar->AimTargetLocation = Target->GetActorLocation();

    const FVector DirToTarget = (Target->GetActorLocation() - AIChar->GetActorLocation()).GetSafeNormal2D();
    const FVector AIDir = AIChar->GetActorForwardVector().GetSafeNormal2D();
    const float Dot = FVector::DotProduct(AIDir, DirToTarget);
    const float Angle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Dot, -1.f, 1.f)));

    if (Angle <= AcceptableAngle)
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
    }
}
