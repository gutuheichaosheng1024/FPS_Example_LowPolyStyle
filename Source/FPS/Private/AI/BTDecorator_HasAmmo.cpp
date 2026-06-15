#include "AI/BTDecorator_HasAmmo.h"
#include "Character/FPS_CharacterBase.h"
#include "AI/FPS_AIController.h"
#include "Weapon/WeaponActor.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTDecorator_HasAmmo::UBTDecorator_HasAmmo()
{
    NodeName = TEXT("Has Ammo");
}

bool UBTDecorator_HasAmmo::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
    AFPS_CharacterBase* Char = Cast<AFPS_CharacterBase>(OwnerComp.GetAIOwner()->GetPawn());
    if (!Char || !Char->CurrentWeapon) return false;

    return Char->CurrentWeapon->HasAnyAmmo();
}
