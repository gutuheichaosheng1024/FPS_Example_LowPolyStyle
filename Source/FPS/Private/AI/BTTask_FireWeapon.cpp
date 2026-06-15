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

    // 无配置 → 直瞄目标
    if (!Config || Config->AimBones.Num() == 0)
    {
        AIChar->AimTargetLocation = TargetLocation;
        return;
    }

    // 计算总命中概率
    float TotalProb = 0.f;
    for (const FAIAimBoneEntry& Entry : Config->AimBones)
    {
        TotalProb += Entry.HitProbability;
    }
    TotalProb = FMath::Clamp(TotalProb, 0.f, 1.f);

    const float Roll = FMath::FRand();

    if (Roll > TotalProb)
    {
        // -------- 未命中：环形随机偏转，保证不低于最小角度 ----------
        // 选一个垂直于 BaseDir 的随机轴
        FVector PerpAxis = FVector::CrossProduct(BaseDir, FVector::UpVector).GetSafeNormal();
        if (PerpAxis.IsNearlyZero())
            PerpAxis = FVector::CrossProduct(BaseDir, FVector::RightVector).GetSafeNormal();

        // 随机方位角绕 BaseDir 旋转偏轴线
        const float Azimuth = FMath::FRand() * 360.f;
        PerpAxis = PerpAxis.RotateAngleAxis(Azimuth, BaseDir);

        // 环形：MinMissAngle ~ MissConeAngle
        const float MissAngle = FMath::RandRange(Config->MinMissAngle, Config->MissConeAngle);
        const FVector MissDir = BaseDir.RotateAngleAxis(MissAngle, PerpAxis);

        AIChar->AimTargetLocation = ShootOrigin + MissDir * 5000.f;
    }
    else
    {
        // -------- 命中：概率加权选骨骼 ----------
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

        FVector BoneLocation = TargetLocation; // 回退到 Actor 位置
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

    // 首次开火前应用命中偏移
    ApplyAimOffset(AIChar, Target);
    AIChar->FireAtTarget(Target);

    FFireWeaponTaskMemory* Memory = reinterpret_cast<FFireWeaponTaskMemory*>(NodeMemory);
    Memory->TimeSinceLastShot = 0.f;

    return EBTNodeResult::InProgress;
}

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

    // 每帧更新瞄准点（保持追瞄目标 + 每发重新掷骰子）
    ApplyAimOffset(AIChar, Target);

    // 半自动：按 FireInterval 重新开火
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

EBTNodeResult::Type UBTTask_FireWeapon::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AFPS_AICharacter* AIChar = Cast<AFPS_AICharacter>(OwnerComp.GetAIOwner()->GetPawn());
    if (AIChar) AIChar->StopFiring();
    return EBTNodeResult::Aborted;
}
