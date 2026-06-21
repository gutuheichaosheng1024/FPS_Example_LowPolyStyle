#include "AI/BTDecorator_HasAmmo.h"
#include "Character/FPS_CharacterBase.h"
#include "AI/FPS_AIController.h"
#include "Weapon/WeaponActor.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTDecorator_HasAmmo::UBTDecorator_HasAmmo()
{
    NodeName = TEXT("Has Ammo");
}

// 检查当前武器弹药：获取角色 → 读取CurrentWeapon->HasAnyAmmo → 返回true/false
// 流程：从OwnerComp获取CharacterBase → 验证角色和CurrentWeapon有效性 → 返回HasAnyAmmo结果
bool UBTDecorator_HasAmmo::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
    AFPS_CharacterBase* Char = Cast<AFPS_CharacterBase>(OwnerComp.GetAIOwner()->GetPawn());
    if (!Char || !Char->CurrentWeapon) return false;

    return Char->CurrentWeapon->HasAnyAmmo();
}
