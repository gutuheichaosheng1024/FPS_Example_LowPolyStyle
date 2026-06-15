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

void UBTService_UpdateAmmo::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    AFPS_CharacterBase* Char = Cast<AFPS_CharacterBase>(OwnerComp.GetAIOwner()->GetPawn());
    if (!Char || !Char->CurrentWeapon) return;

    UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
    if (!BB) return;

    BB->SetValueAsBool(AFPS_AIController::BBKey_HasAmmo, Char->CurrentWeapon->HasAnyAmmo());
}
