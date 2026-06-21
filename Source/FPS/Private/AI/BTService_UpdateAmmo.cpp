#include "AI/BTService_UpdateAmmo.h"
#include "Character/FPS_CharacterBase.h"
#include "AI/FPS_AIController.h"
#include "Weapon/WeaponActor.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTService_UpdateAmmo::UBTService_UpdateAmmo()
{
    NodeName = TEXT("Update Ammo");
    Interval = 0.5f;
}

// 更新弹药状态到黑板：读取角色当前武器的HasAnyAmmo → 写入BBKey_HasAmmo
// 流程：获取CharacterBase → 验证CurrentWeapon → 读取HasAnyAmmo → BB->SetValueAsBool
void UBTService_UpdateAmmo::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    AFPS_CharacterBase* Char = Cast<AFPS_CharacterBase>(OwnerComp.GetAIOwner()->GetPawn());
    if (!Char || !Char->CurrentWeapon) return;

    UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
    if (!BB) return;

    BB->SetValueAsBool(AFPS_AIController::BBKey_HasAmmo, Char->CurrentWeapon->HasAnyAmmo());
}
