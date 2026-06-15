#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_ReloadWeapon.generated.h"

UCLASS()
class FPS_API UBTTask_ReloadWeapon : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_ReloadWeapon();

protected:
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
