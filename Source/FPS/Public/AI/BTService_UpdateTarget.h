#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_UpdateTarget.generated.h"

UCLASS()
class FPS_API UBTService_UpdateTarget : public UBTService
{
    GENERATED_BODY()

public:
    UBTService_UpdateTarget();

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FBlackboardKeySelector TargetActorKey;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FBlackboardKeySelector HasLineOfSightKey;

protected:
    virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};
