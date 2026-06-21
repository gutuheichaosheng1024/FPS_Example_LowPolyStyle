#include "AI/BTTask_FireWeapon.h"
#include "Character/FPS_AICharacter.h"
#include "AI/FPS_AIController.h"
#include "Weapon/AIAttackConfig.h"
#include "Weapon/WeaponActor.h"
#include "Weapon/WeaponDataConfig.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_FireWeapon::UBTTask_FireWeapon()
{
    NodeName = TEXT("Fire Weapon");
    bNotifyTick = true;
}

// 根据命中概率计算瞄准偏移：命中时概率加权选骨骼，未命中时环形随机偏转
// 流程：计算总命中概率 → 随机掷骰 → 未命中则环形随机偏转方向 → 命中则概率加权选目标骨骼 → 写入AIChar->AimTargetLocation
void UBTTask_FireWeapon::ApplyAimOffset(AFPS_AICharacter* AIChar, const AActor* Target) const
{
    if (!AIChar || !Target)
    {
        return;
    }

    const FVector ShootOrigin = AIChar->GetShootLocation();
    const FVector TargetLocation = Target->GetActorLocation();
    const FVector BaseDir = (TargetLocation - ShootOrigin).GetSafeNormal();

    const UAIAttackConfig* Config = AIChar->AttackConfig;

    if (!Config || Config->AimBones.Num() == 0)
    {
        AIChar->AimTargetLocation = TargetLocation;
        return;
    }

    float TotalProb = 0.f;
    for (const FAIAimBoneEntry& Entry : Config->AimBones)
    {
        TotalProb += Entry.HitProbability;
    }
    TotalProb = FMath::Clamp(TotalProb, 0.f, 1.f);

    const float Roll = FMath::FRand();

    if (Roll > TotalProb)
    {
        FVector PerpAxis = FVector::CrossProduct(BaseDir, FVector::UpVector).GetSafeNormal();
        if (PerpAxis.IsNearlyZero())
            PerpAxis = FVector::CrossProduct(BaseDir, FVector::RightVector).GetSafeNormal();

        const float Azimuth = FMath::FRand() * 360.f;
        PerpAxis = PerpAxis.RotateAngleAxis(Azimuth, BaseDir);

        const float MissAngle = FMath::RandRange(Config->MinMissAngle, Config->MissConeAngle);
        const FVector MissDir = BaseDir.RotateAngleAxis(MissAngle, PerpAxis);

        AIChar->AimTargetLocation = ShootOrigin + MissDir * 5000.f;
    }
    else
    {
        float Cumulative = 0.f;
        FName SelectedBone = NAME_None;
        for (const FAIAimBoneEntry& Entry : Config->AimBones)
        {
            Cumulative += Entry.HitProbability;
            if (Roll <= Cumulative)
            {
                SelectedBone = Entry.BoneName;
                break;
            }
        }

        FVector BoneLocation = TargetLocation;
        if (SelectedBone != NAME_None)
        {
            if (const USkeletalMeshComponent* TargetMesh = Target->FindComponentByClass<USkeletalMeshComponent>())
            {
                const FVector SocketLoc = TargetMesh->GetSocketLocation(SelectedBone);
                if (!SocketLoc.IsZero())
                {
                    BoneLocation = SocketLoc;
                }
            }
        }
        AIChar->AimTargetLocation = BoneLocation;
    }
}

// 开始开火：验证AI角色/武器/目标/弹药 → 应用瞄准偏移 → 开始射击 → 进入InProgress持续Tick
// 流程：获取AICharacter → 验证CurrentWeapon → 从黑板获取TargetActor → 检查弹药 → ApplyAimOffset → FireAtTarget → 初始化计时器
EBTNodeResult::Type UBTTask_FireWeapon::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AFPS_AICharacter* AIChar = Cast<AFPS_AICharacter>(OwnerComp.GetAIOwner()->GetPawn());
    if (!AIChar || !AIChar->CurrentWeapon) return EBTNodeResult::Failed;

    UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
    if (!BB) return EBTNodeResult::Failed;

    AActor* Target = Cast<AActor>(BB->GetValueAsObject(AFPS_AIController::BBKey_TargetActor));
    if (!Target) return EBTNodeResult::Failed;

    if (!AIChar->CurrentWeapon->HasAnyAmmo())
        return EBTNodeResult::Failed;

    ApplyAimOffset(AIChar, Target);
    AIChar->FireAtTarget(Target);

    FFireWeaponTaskMemory* Memory = reinterpret_cast<FFireWeaponTaskMemory*>(NodeMemory);
    Memory->TimeSinceLastShot = 0.f;

    return EBTNodeResult::InProgress;
}

// 持续开火Tick：验证目标/弹药 → 更新瞄准偏移 → 半自动模式下按FireInterval重新开火
// 流程：验证AIChar/Weapon/BB/Target → 检查弹药 → ApplyAimOffset → 半自动模式累计时间并按FireInterval触发FireAtTarget → 失败条件中止任务
void UBTTask_FireWeapon::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    AFPS_AICharacter* AIChar = Cast<AFPS_AICharacter>(OwnerComp.GetAIOwner()->GetPawn());
    if (!AIChar || !AIChar->CurrentWeapon)
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
    if (!BB)
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    AActor* Target = Cast<AActor>(BB->GetValueAsObject(AFPS_AIController::BBKey_TargetActor));
    if (!Target)
    {
        AIChar->StopFiring();
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    if (!AIChar->CurrentWeapon->HasAnyAmmo())
    {
        AIChar->StopFiring();
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    AWeaponActor* Weapon = AIChar->CurrentWeapon;
    UWeaponDataConfig* Config = Weapon->DataConfig;
    if (!Config)
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    ApplyAimOffset(AIChar, Target);

    if (Config->FireMode == EWeaponFireMode::SemiAuto)
    {
        FFireWeaponTaskMemory* Memory = reinterpret_cast<FFireWeaponTaskMemory*>(NodeMemory);
        Memory->TimeSinceLastShot += DeltaSeconds;

        if (Memory->TimeSinceLastShot >= Config->FireInterval)
        {
            AIChar->FireAtTarget(Target);
            Memory->TimeSinceLastShot = 0.f;
        }
    }
}

// 中止开火任务：停止AI射击并返回Aborted
// 流程：获取AICharacter → StopFiring → 返回EBTNodeResult::Aborted
EBTNodeResult::Type UBTTask_FireWeapon::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AFPS_AICharacter* AIChar = Cast<AFPS_AICharacter>(OwnerComp.GetAIOwner()->GetPawn());
    if (AIChar) AIChar->StopFiring();
    return EBTNodeResult::Aborted;
}
