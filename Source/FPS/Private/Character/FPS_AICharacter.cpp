#include "Character/FPS_AICharacter.h"
#include "Weapon/WeaponActor.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"

AFPS_AICharacter::AFPS_AICharacter()
{
}

void AFPS_AICharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AFPS_AICharacter, AimTargetLocation);
}

void AFPS_AICharacter::BeginPlay()
{
    Super::BeginPlay();

    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->bUseControllerDesiredRotation = true;
        MoveComp->bOrientRotationToMovement = false;
    }
}

void AFPS_AICharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    UpdateAimDirection(DeltaTime);
}

// ---------- 多态接口 ----------
FVector AFPS_AICharacter::GetShootLocation() const
{
    if (USkeletalMeshComponent* CharMesh = GetMesh())
    {
        if (ShootBoneName != NAME_None)
        {
            const FVector SocketLoc = CharMesh->GetSocketLocation(ShootBoneName);
            if (!SocketLoc.IsZero())
                return SocketLoc;
        }
    }
    return GetActorLocation();
}

FRotator AFPS_AICharacter::GetShootRotation() const
{
    return (AimTargetLocation - GetShootLocation()).Rotation();
}

// ---------- 瞄准 ----------
void AFPS_AICharacter::UpdateAimDirection(float DeltaTime)
{
    FRotator DesiredRotation = (AimTargetLocation - GetActorLocation()).Rotation();
    DesiredRotation.Pitch = 0.f;
    SetActorRotation(FMath::RInterpTo(GetActorRotation(), DesiredRotation, DeltaTime, AimRotationSpeed));
}

void AFPS_AICharacter::AimAtTarget(AActor* Target)
{
    if (Target)
    {
        AimTargetLocation = Target->GetActorLocation();
        Aim();
    }
}

// ---------- 射击 ----------
void AFPS_AICharacter::FireAtTarget(AActor* Target)
{
    if (!CurrentWeapon) return;

    CurrentTarget = Target;
    // AimTargetLocation 由 BT 节点负责设置（ApplyAimOffset），这里不覆盖

    if (!bIsFiring)
    {
        bIsFiring = true;
        CurrentWeapon->Fire();
    }
}

void AFPS_AICharacter::StopFiring()
{
    if (!CurrentWeapon) return;
    bIsFiring = false;
    CurrentWeapon->StopFire();
}

void AFPS_AICharacter::ReloadWeapon()
{
    if (CurrentWeapon)
        CurrentWeapon->Reload();
}

void AFPS_AICharacter::HandleDeath()
{
    // 服务器权威：停止射击 + 停止行为树
    StopFiring();
    if (AAIController* AICon = Cast<AAIController>(Controller))
    {
        AICon->BrainComponent->StopLogic(TEXT("Death"));
    }

    Super::HandleDeath();  // 销毁武器 + 禁用碰撞 + 禁用移动

    // 延迟销毁
    if (DeathDestroyDelay > 0.f)
        SetLifeSpan(DeathDestroyDelay);
    else
        Destroy();
}

void AFPS_AICharacter::Multicast_OnDeath_Implementation()
{
    Super::Multicast_OnDeath_Implementation();

    // 客户端表现：布娃娃
    if (USkeletalMeshComponent* MeshTP = GetMesh())
    {
        MeshTP->SetSimulatePhysics(true);
        MeshTP->SetCollisionProfileName(TEXT("Ragdoll"));
    }
}
