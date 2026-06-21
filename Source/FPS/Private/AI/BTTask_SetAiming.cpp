#include "AI/BTTask_SetAiming.h"
#include "Character/FPS_CharacterBase.h"
#include "AIController.h"

UBTTask_SetAiming::UBTTask_SetAiming()
{
    NodeName = TEXT("Set Aiming");
}

// 设置AI瞄准状态：根据bAim标志调用Aim()或StopAim()
// 流程：从OwnerComp获取CharacterBase → 验证有效性 → bAim为true则Aim()否则StopAim() → 返回Succeeded
EBTNodeResult::Type UBTTask_SetAiming::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AFPS_CharacterBase* Char = Cast<AFPS_CharacterBase>(OwnerComp.GetAIOwner()->GetPawn());
    if (!Char) return EBTNodeResult::Failed;

    if (bAim)
        Char->Aim();
    else
        Char->StopAim();
    return EBTNodeResult::Succeeded;
}

// 中止瞄准任务：强制停止AI瞄准
// 流程：获取CharacterBase → 验证有效性 → StopAim() → 返回Aborted
EBTNodeResult::Type UBTTask_SetAiming::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AFPS_CharacterBase* Char = Cast<AFPS_CharacterBase>(OwnerComp.GetAIOwner()->GetPawn());
    if (Char) Char->StopAim();
    return EBTNodeResult::Aborted;
}
