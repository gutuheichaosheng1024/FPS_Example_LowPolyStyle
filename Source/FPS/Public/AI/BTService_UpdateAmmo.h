#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_UpdateAmmo.generated.h"

/**
 * UBTService_UpdateAmmo — Behavior Tree service that periodically updates the HasAmmo Blackboard key
 *
 * 职责：以0.5s间隔检查AI当前武器弹药状态，将结果写入黑板HasAmmo键供行为树Decorator使用
 * 使用：AFPS_CharacterBase, AWeaponActor, AFPS_AIController, UBlackboardComponent
 */
UCLASS()
class FPS_API UBTService_UpdateAmmo : public UBTService
{
    GENERATED_BODY()

public:
    UBTService_UpdateAmmo();

protected:
    virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};
