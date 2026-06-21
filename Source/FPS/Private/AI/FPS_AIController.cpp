#include "AI/FPS_AIController.h"
#include "Character/FPS_AICharacter.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AIPerceptionSystem.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"

const FName AFPS_AIController::BBKey_TargetActor(TEXT("TargetActor"));
const FName AFPS_AIController::BBKey_LastKnownLocation(TEXT("LastKnownLocation"));
const FName AFPS_AIController::BBKey_HasAmmo(TEXT("HasAmmo"));
const FName AFPS_AIController::BBKey_HasLineOfSight(TEXT("HasLineOfSight"));
const FName AFPS_AIController::BBKey_PatrolIndex(TEXT("PatrolIndex"));
const FName AFPS_AIController::BBKey_DistanceToTarget(TEXT("DistanceToTarget"));
const FName AFPS_AIController::BBKey_InFireRange(TEXT("InFireRange"));
const FName AFPS_AIController::BBKey_ShouldEngage(TEXT("ShouldEngage"));

AFPS_AIController::AFPS_AIController()
{
    UAIPerceptionComponent* PercComp = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComponent"));
    SetPerceptionComponent(*PercComp);

    SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
    SightConfig->SightRadius = 2000.f;
    SightConfig->LoseSightRadius = 2500.f;
    SightConfig->PeripheralVisionAngleDegrees = 60.f;
    SightConfig->DetectionByAffiliation.bDetectEnemies = true;
    SightConfig->DetectionByAffiliation.bDetectNeutrals = false;
    SightConfig->DetectionByAffiliation.bDetectFriendlies = false;

    HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
    HearingConfig->HearingRange = 3000.f;
    HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
    HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
    HearingConfig->DetectionByAffiliation.bDetectFriendlies = false;

    PercComp->ConfigureSense(*SightConfig);
    PercComp->ConfigureSense(*HearingConfig);
    PercComp->SetDominantSense(SightConfig->GetSenseImplementation());
}

void AFPS_AIController::BeginPlay()
{
    Super::BeginPlay();
}

// 接管Pawn时执行：同步团队ID → 运行行为树 → 绑定感知回调
// 流程：Super::OnPossess → 从AICharacter同步GenericTeamId → 运行BehaviorTree → 绑定OnTargetPerceptionUpdated
void AFPS_AIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    if (AFPS_AICharacter* AIChar = Cast<AFPS_AICharacter>(InPawn))
    {
        SetGenericTeamId(AIChar->GetGenericTeamId());
    }

    if (BehaviorTreeAsset)
    {
        RunBehaviorTree(BehaviorTreeAsset);
    }

    UAIPerceptionComponent* PercComp = GetPerceptionComponent();
    if (PercComp)
    {
        PercComp->OnTargetPerceptionUpdated.AddDynamic(
            this, &AFPS_AIController::OnTargetPerceptionUpdated);
    }
}

// 处理AI感知目标更新：视觉写入TargetActor+LastKnownLocation，听觉仅写入LastKnownLocation，丢失时清除TargetActor
// 流程：检查刺激类型(视觉/听觉) → 感测成功则写入LastKnownLocation → 视觉额外写入TargetActor → 丢失时若为视觉且当前目标匹配则清除TargetActor
void AFPS_AIController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
    UBlackboardComponent* BB = GetBlackboardComponent();
    if (!BB || !Actor) return;

    const bool bIsSight = Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>();
    const bool bIsHearing = Stimulus.Type == UAISense::GetSenseID<UAISense_Hearing>();

    if (Stimulus.WasSuccessfullySensed())
    {
        BB->SetValueAsVector(BBKey_LastKnownLocation, Stimulus.StimulusLocation);

        if (bIsSight)
        {
            BB->SetValueAsObject(BBKey_TargetActor, Actor);
        }
    }
    else
    {
        if (bIsSight)
        {
            UObject* CurrentTarget = BB->GetValueAsObject(BBKey_TargetActor);
            if (CurrentTarget == Actor)
            {
                BB->ClearValue(BBKey_TargetActor);
            }
        }
    }
}
