#include "AI/BTTask_ReloadWeapon.h"
#include "Character/FPS_AICharacter.h"
#include "AIController.h"

UBTTask_ReloadWeapon::UBTTask_ReloadWeapon()
{
    NodeName = TEXT("Reload Weapon");
}

EBTNodeResult::Type UBTTask_ReloadWeapon::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AFPS_AICharacter* AIChar = Cast<AFPS_AICharacter>(OwnerComp.GetAIOwner()->GetPawn());
    if (!AIChar) return EBTNodeResult::Failed;

    AIChar->ReloadWeapon();
    return EBTNodeResult::Succeeded;
}
