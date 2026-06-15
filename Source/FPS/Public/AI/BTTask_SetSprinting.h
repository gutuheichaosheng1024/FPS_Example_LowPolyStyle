#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_SetSprinting.generated.h"

UCLASS()
class FPS_API UBTTask_SetSprinting : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_SetSprinting();

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bSprint = true;

protected:
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
    virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
