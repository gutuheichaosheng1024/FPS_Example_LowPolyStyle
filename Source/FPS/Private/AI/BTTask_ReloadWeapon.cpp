#include "AI/BTTask_ReloadWeapon.h"
#include "Character/FPS_AICharacter.h"
#include "AIController.h"

UBTTask_ReloadWeapon::UBTTask_ReloadWeapon()
{
    NodeName = TEXT("Reload Weapon");
}

// 执行AI武器换弹：获取AI角色 → 调用ReloadWeapon → 返回成功
// 流程：从OwnerComp获取AICharacter → 验证有效性 → AIChar->ReloadWeapon() → EBTNodeResult::Succeeded
EBTNodeResult::Type UBTTask_ReloadWeapon::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AFPS_AICharacter* AIChar = Cast<AFPS_AICharacter>(OwnerComp.GetAIOwner()->GetPawn());
    if (!AIChar) return EBTNodeResult::Failed;

    AIChar->ReloadWeapon();
    return EBTNodeResult::Succeeded;
}
