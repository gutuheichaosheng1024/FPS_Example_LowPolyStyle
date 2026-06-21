#include "AI/BTTask_SetSprinting.h"
#include "Character/FPS_CharacterBase.h"
#include "AIController.h"

UBTTask_SetSprinting::UBTTask_SetSprinting()
{
    NodeName = TEXT("Set Sprinting");
}

// 设置AI冲刺状态：根据bSprint标志调用Sprint()或StopSprint()
// 流程：从OwnerComp获取CharacterBase → 验证有效性 → bSprint为true则Sprint()否则StopSprint() → 返回Succeeded
EBTNodeResult::Type UBTTask_SetSprinting::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AFPS_CharacterBase* Char = Cast<AFPS_CharacterBase>(OwnerComp.GetAIOwner()->GetPawn());
    if (!Char) return EBTNodeResult::Failed;

    if (bSprint)
        Char->Sprint();
    else
        Char->StopSprint();
    return EBTNodeResult::Succeeded;
}

// 中止冲刺任务：强制停止AI冲刺
// 流程：获取CharacterBase → 验证有效性 → StopSprint() → 返回Aborted
EBTNodeResult::Type UBTTask_SetSprinting::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AFPS_CharacterBase* Char = Cast<AFPS_CharacterBase>(OwnerComp.GetAIOwner()->GetPawn());
    if (Char) Char->StopSprint();
    return EBTNodeResult::Aborted;
}
