#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_UpdateAmmo.generated.h"

UCLASS()
class FPS_API UBTService_UpdateAmmo : public UBTService
{
    GENERATED_BODY()

public:
    UBTService_UpdateAmmo();

protected:
    virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};
