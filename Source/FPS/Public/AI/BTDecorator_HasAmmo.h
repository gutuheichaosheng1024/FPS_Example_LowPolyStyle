#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_HasAmmo.generated.h"

UCLASS()
class FPS_API UBTDecorator_HasAmmo : public UBTDecorator
{
    GENERATED_BODY()

public:
    UBTDecorator_HasAmmo();

protected:
    virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
};
