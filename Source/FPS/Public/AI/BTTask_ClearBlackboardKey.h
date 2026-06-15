#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_ClearBlackboardKey.generated.h"

UCLASS()
class FPS_API UBTTask_ClearBlackboardKey : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_ClearBlackboardKey();

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FBlackboardKeySelector KeyToClear;

protected:
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
