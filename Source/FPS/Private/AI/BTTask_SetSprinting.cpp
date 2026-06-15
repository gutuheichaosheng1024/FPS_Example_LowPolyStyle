#include "AI/BTTask_SetSprinting.h"
#include "Character/FPS_CharacterBase.h"
#include "AIController.h"

UBTTask_SetSprinting::UBTTask_SetSprinting()
{
    NodeName = TEXT("Set Sprinting");
}

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

EBTNodeResult::Type UBTTask_SetSprinting::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AFPS_CharacterBase* Char = Cast<AFPS_CharacterBase>(OwnerComp.GetAIOwner()->GetPawn());
    if (Char) Char->StopSprint();
    return EBTNodeResult::Aborted;
}
