#include "AI/BTTask_SetAiming.h"
#include "Character/FPS_CharacterBase.h"
#include "AIController.h"

UBTTask_SetAiming::UBTTask_SetAiming()
{
    NodeName = TEXT("Set Aiming");
}

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

EBTNodeResult::Type UBTTask_SetAiming::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AFPS_CharacterBase* Char = Cast<AFPS_CharacterBase>(OwnerComp.GetAIOwner()->GetPawn());
    if (Char) Char->StopAim();
    return EBTNodeResult::Aborted;
}
