#include "AI/BTTask_FindRandomPatrolPoint.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "NavigationSystem.h"
#include "AI/Navigation/NavigationTypes.h"

UBTTask_FindRandomPatrolPoint::UBTTask_FindRandomPatrolPoint()
{
    NodeName = TEXT("Find Random Patrol Point");
}

EBTNodeResult::Type UBTTask_FindRandomPatrolPoint::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AAIController* AICon = OwnerComp.GetAIOwner();
    if (!AICon)
    {
        return EBTNodeResult::Failed;
    }

    APawn* Pawn = AICon->GetPawn();
    if (!Pawn)
    {
        return EBTNodeResult::Failed;
    }

    UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
    if (!BB || OutputLocationKey.SelectedKeyName == NAME_None)
    {
        return EBTNodeResult::Failed;
    }

    UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(Pawn->GetWorld());
    if (!NavSys)
    {
        return EBTNodeResult::Failed;
    }

    const FVector Origin = Pawn->GetActorLocation();
    FNavLocation NavLocation;

    if (NavSys->GetRandomReachablePointInRadius(Origin, SearchRadius, NavLocation))
    {
        BB->SetValueAsVector(OutputLocationKey.SelectedKeyName, NavLocation.Location);
        return EBTNodeResult::Succeeded;
    }

    return EBTNodeResult::Failed;
}
