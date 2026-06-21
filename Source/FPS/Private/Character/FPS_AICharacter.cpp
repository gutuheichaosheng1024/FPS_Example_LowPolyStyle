#include "Character/FPS_AICharacter.h"
#include "Weapon/WeaponActor.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"

// 构造函数：分配唯一 TeamId
// 流程：通过 GetNextTeamId 获取唯一队伍 ID 并赋值
AFPS_AICharacter::AFPS_AICharacter()
{
    TeamId = FGenericTeamId(AFPS_CharacterBase::GetNextTeamId());
}

// 注册需要网络复制的属性
// 流程：调用父类注册 → 注册 AimTargetLocation
void AFPS_AICharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AFPS_AICharacter, AimTargetLocation);
}

// AI 角色初始化
// 流程：调用父类 BeginPlay → 配置 MovementComponent 使用控制器期望旋转方式
void AFPS_AICharacter::BeginPlay()
{
    Super::BeginPlay();

    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->bUseControllerDesiredRotation = true;
        MoveComp->bOrientRotationToMovement = false;
    }
}

// AI 角色每帧更新
// 流程：调用父类 Tick → 更新瞄准方向旋转插值
void AFPS_AICharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    UpdateAimDirection(DeltaTime);
}

// 获取射击起点（AI：骨骼插槽位置或 Actor 位置）
// 流程：优先从 ShootBoneName 对应插槽获取位置 → 无效则返回 ActorLocation
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

// 获取射击旋转（AI：指向瞄准目标位置的方向）
// 流程：计算从射击起点指向 AimTargetLocation 的方向向量 → 转为 Rotator
FRotator AFPS_AICharacter::GetShootRotation() const
{
    return (AimTargetLocation - GetShootLocation()).Rotation();
}

// 更新角色面朝瞄准目标方向的插值旋转
// 流程：计算 Actor 到 AimTargetLocation 的水平方向 → RInterpTo 平滑旋转 → 忽略 Pitch
void AFPS_AICharacter::UpdateAimDirection(float DeltaTime)
{
    FRotator DesiredRotation = (AimTargetLocation - GetActorLocation()).Rotation();
    DesiredRotation.Pitch = 0.f;
    SetActorRotation(FMath::RInterpTo(GetActorRotation(), DesiredRotation, DeltaTime, AimRotationSpeed));
}

// 瞄准目标Actor
// 流程：更新 AimTargetLocation 为目标位置 → 调用 Aim 进入瞄准状态
void AFPS_AICharacter::AimAtTarget(AActor* Target)
{
    if (Target)
    {
        AimTargetLocation = Target->GetActorLocation();
        Aim();
    }
}

// 向目标开火
// 流程：检查武器有效性 → 缓存当前目标 → 未开火时标记开火并调用 Weapon->Fire
void AFPS_AICharacter::FireAtTarget(AActor* Target)
{
    if (!CurrentWeapon) return;

    CurrentTarget = Target;

    if (!bIsFiring)
    {
        bIsFiring = true;
        CurrentWeapon->Fire();
    }
}

// 停止射击
// 流程：检查武器有效性 → 清除开火标记 → 调用 Weapon->StopFire
void AFPS_AICharacter::StopFiring()
{
    if (!CurrentWeapon) return;
    bIsFiring = false;
    CurrentWeapon->StopFire();
}

// AI 换弹
// 流程：检查武器有效性 → 调用 Weapon->Reload
void AFPS_AICharacter::ReloadWeapon()
{
    if (CurrentWeapon)
        CurrentWeapon->Reload();
}

// AI 死亡处理（服务器权威）
// 流程：停止射击 → 停止行为树逻辑 → 调用基类 HandleDeath（禁用碰撞/移动 + 广播 OnKilled）
void AFPS_AICharacter::HandleDeath()
{
    StopFiring();
    if (AAIController* AICon = Cast<AAIController>(Controller))
    {
        AICon->BrainComponent->StopLogic(TEXT("Death"));
    }

    Super::HandleDeath();
}

// AI 死亡多播表现
// 流程：调用基类 Multicast_OnDeath → TP 网格启用布娃娃物理
void AFPS_AICharacter::Multicast_OnDeath_Implementation()
{
    Super::Multicast_OnDeath_Implementation();

    if (USkeletalMeshComponent* MeshTP = GetMesh())
    {
        MeshTP->SetSimulatePhysics(true);
        MeshTP->SetCollisionProfileName(TEXT("Ragdoll"));
    }
}
