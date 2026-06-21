#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionComponent.h"
#include "FPS_AIController.generated.h"

class UAISenseConfig_Sight;
class UAISenseConfig_Hearing;
class UBehaviorTree;

/**
 * AFPS_AIController — AI controller with sight/hearing perception and Blackboard key management
 *
 * 职责：管理AI感知（视觉20m/听觉30m）、Blackboard键值定义、行为树运行、目标感知回调与重生逻辑
 * 使用：AFPS_AICharacter, UAISenseConfig_Sight, UAISenseConfig_Hearing, UBehaviorTree, UBlackboardComponent
 */
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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Respawn", meta = (ClampMin = "0.5", ClampMax = "60.0"))
    float RespawnTime = 5.f;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Perception")
    TObjectPtr<UAISenseConfig_Sight> SightConfig;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Perception")
    TObjectPtr<UAISenseConfig_Hearing> HearingConfig;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Behavior")
    TObjectPtr<UBehaviorTree> BehaviorTreeAsset;
};
