#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionComponent.h"
#include "FPS_AIController.generated.h"

class UAISenseConfig_Sight;
class UAISenseConfig_Hearing;
class UBehaviorTree;

UCLASS()
class FPS_API AFPS_AIController : public AAIController
{
    GENERATED_BODY()

public:
    AFPS_AIController();
    virtual void BeginPlay() override;
    virtual void OnPossess(APawn* InPawn) override;

    UFUNCTION()
    void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

    static const FName BBKey_TargetActor;
    static const FName BBKey_LastKnownLocation;
    static const FName BBKey_HasAmmo;
    static const FName BBKey_HasLineOfSight;
    static const FName BBKey_PatrolIndex;
    static const FName BBKey_DistanceToTarget;
    static const FName BBKey_InFireRange;
    static const FName BBKey_ShouldEngage;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Perception")
    TObjectPtr<UAISenseConfig_Sight> SightConfig;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Perception")
    TObjectPtr<UAISenseConfig_Hearing> HearingConfig;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Behavior")
    TObjectPtr<UBehaviorTree> BehaviorTreeAsset;
};
