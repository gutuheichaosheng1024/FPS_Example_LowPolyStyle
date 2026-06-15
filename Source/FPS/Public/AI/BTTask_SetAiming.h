#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_SetAiming.generated.h"

UCLASS()
class FPS_API UBTTask_SetAiming : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_SetAiming();

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bAim = true;

protected:
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
    virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
