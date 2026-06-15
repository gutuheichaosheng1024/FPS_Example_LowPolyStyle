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

void AFPS_AIController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
    UBlackboardComponent* BB = GetBlackboardComponent();
    if (!BB || !Actor) return;

    const bool bIsSight = Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>();
    const bool bIsHearing = Stimulus.Type == UAISense::GetSenseID<UAISense_Hearing>();

    if (Stimulus.WasSuccessfullySensed())
    {
        // 听觉和视觉都记录位置
        BB->SetValueAsVector(BBKey_LastKnownLocation, Stimulus.StimulusLocation);

        // 只有视觉感知才写入 TargetActor（触发战斗），听觉只记录位置（触发调查）
        if (bIsSight)
        {
            BB->SetValueAsObject(BBKey_TargetActor, Actor);
        }
    }
    else
    {
        // 丢失时，视觉丢失才清除目标
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
