#include "Character/FPS_CharacterBase.h"
#include "Weapon/WeaponActor.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

static uint8 GNextTeamId = 0;

// 返回下一个唯一TeamId
// 流程：读取静态计数器当前值 → 自增计数器 → 返回旧值
uint8 AFPS_CharacterBase::GetNextTeamId()
{
    return GNextTeamId++;
}

// 构造函数，启用网络复制
// 流程：设置 bReplicates = true
AFPS_CharacterBase::AFPS_CharacterBase()
{
    bReplicates = true;
}

// 注册需要网络复制的属性
// 流程：调用父类注册 → 注册血量/状态/武器相关复制属性
void AFPS_CharacterBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AFPS_CharacterBase, MaxHealth);
    DOREPLIFETIME(AFPS_CharacterBase, CurrentHealth);
    DOREPLIFETIME(AFPS_CharacterBase, Sprinting);
    DOREPLIFETIME(AFPS_CharacterBase, Aiming);
    DOREPLIFETIME(AFPS_CharacterBase, Reloading);
    DOREPLIFETIME(AFPS_CharacterBase, Viewing);
    DOREPLIFETIME(AFPS_CharacterBase, CurrentWeapon);
    DOREPLIFETIME_CONDITION(AFPS_CharacterBase, WeaponInventory, COND_OwnerOnly);
}

// 客户端收到生命值更新后的回调
// 流程：预留，可在此更新UI或播放受伤特效
void AFPS_CharacterBase::OnRep_CurrentHealth()
{
}

// 客户端收到当前武器更新后的回调
// 流程：遍历武器库存 → 激活 CurrentWeapon → 停用其他武器
void AFPS_CharacterBase::OnRep_CurrentWeapon()
{
    for (AWeaponActor* Weapon : WeaponInventory)
    {
        if (!Weapon) continue;
        if (Weapon == CurrentWeapon)
        {
            Weapon->ActivateWeapon();
        }
        else
        {
            Weapon->DeactivateWeapon();
        }
    }
}

// 客户端收到冲刺状态更新后的回调
// 流程：重新应用移动速度
void AFPS_CharacterBase::OnRep_Sprinting()
{
    ApplyMovementSpeed();
}

// 客户端收到瞄准状态更新后的回调
// 流程：重新应用移动速度
void AFPS_CharacterBase::OnRep_Aiming()
{
    ApplyMovementSpeed();
}

// 客户端收到换弹状态更新后的回调
// 流程：重新应用移动速度
void AFPS_CharacterBase::OnRep_Reloading()
{
    ApplyMovementSpeed();
}

// 客户端收到检视状态更新后的回调
// 流程：重新应用移动速度
void AFPS_CharacterBase::OnRep_Viewing()
{
    ApplyMovementSpeed();
}

// 角色初始化
// 流程：设置初始血量 → 设置马克速度 → 服务器端生成默认武器并装备
void AFPS_CharacterBase::BeginPlay()
{
    Super::BeginPlay();

    CurrentHealth = MaxHealth;
    bIsDead = false;

    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->MaxWalkSpeed = WalkSpeed;
    }

    if (DefaultWeaponClass && HasAuthority())
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = this;
        AWeaponActor* Weapon = GetWorld()->SpawnActor<AWeaponActor>(DefaultWeaponClass, SpawnParams);
        if (Weapon)
        {
            EquipWeaponActor(Weapon);
        }
    }
}

// 每帧更新
// 流程：更新着地状态 → 更新脚步声计时器
void AFPS_CharacterBase::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        bIsGrounded = !MoveComp->IsFalling();
    }

    UpdateFootsteps();
}

// 处理伤害（仅服务器执行）
// 流程：检查是否已死亡 → 仅 Authority 执行 → 扣除血量并钳制到 [0, MaxHealth] → 死亡时缓存击杀者 → HandleDeath → Multicast_OnDeath → 广播 OnDeath 委托
float AFPS_CharacterBase::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
    AController* EventInstigator, AActor* DamageCauser)
{
    if (bIsDead) return 0.f;
    if (!HasAuthority()) return 0.f;

    const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
    CurrentHealth = FMath::Max(CurrentHealth - ActualDamage, 0.f);

    if (CurrentHealth <= 0.f)
    {
        bIsDead = true;
        LastDamageInstigator = EventInstigator;
        HandleDeath();
        Multicast_OnDeath();
        OnDeath.Broadcast();
    }

    return ActualDamage;
}

// 处理死亡（服务器权威）
// 流程：禁用碰撞胶囊体 → 禁用角色移动 → 停用当前武器并清空库存 → 广播 OnKilled 委托
void AFPS_CharacterBase::HandleDeath()
{
    if (UCapsuleComponent* Capsule = GetCapsuleComponent())
    {
        Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->DisableMovement();
    }

    if (CurrentWeapon)
    {
        CurrentWeapon->DeactivateWeapon();
    }
    CurrentWeapon = nullptr;
    WeaponInventory.Empty();

    OnKilled.Broadcast(this, LastDamageInstigator);
}

// 多播死亡表现（所有客户端执行）
// 流程：禁用碰撞胶囊体 → 禁用角色移动
void AFPS_CharacterBase::Multicast_OnDeath_Implementation()
{
    if (UCapsuleComponent* Capsule = GetCapsuleComponent())
    {
        Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->DisableMovement();
    }
}

// 验证拾取请求参数
// 流程：检查 Weapon 指针非空
bool AFPS_CharacterBase::Server_RequestPickup_Validate(AWeaponActor* Weapon)
{
    return Weapon != nullptr;
}

// 服务器执行拾取
// 流程：调用 EquipWeaponActor 装备武器
void AFPS_CharacterBase::Server_RequestPickup_Implementation(AWeaponActor* Weapon)
{
    EquipWeaponActor(Weapon);
}

// 开始冲刺（基类默认：直接设置状态）
// 流程：检查换弹中则忽略 → 设置 Sprinting = true → 应用移动速度
void AFPS_CharacterBase::Sprint()
{
    if (Reloading) return;
    Sprinting = true;
    ApplyMovementSpeed();
}

// 停止冲刺
// 流程：设置 Sprinting = false → 应用移动速度
void AFPS_CharacterBase::StopSprint()
{
    Sprinting = false;
    ApplyMovementSpeed();
}

// 开始瞄准
// 流程：设置 Aiming = true → 应用移动速度
void AFPS_CharacterBase::Aim()
{
    Aiming = true;
    ApplyMovementSpeed();
}

// 停止瞄准
// 流程：设置 Aiming = false → 应用移动速度
void AFPS_CharacterBase::StopAim()
{
    Aiming = false;
    ApplyMovementSpeed();
}

// 验证冲刺 RPC 参数
// 流程：检查角色未死亡
bool AFPS_CharacterBase::Server_SetSprinting_Validate(bool bNewSprinting)
{
    return !bIsDead;
}

// 服务器执行冲刺状态同步
// 流程：更新 Sprinting → 立即应用移动速度
void AFPS_CharacterBase::Server_SetSprinting_Implementation(bool bNewSprinting)
{
    Sprinting = bNewSprinting;
    ApplyMovementSpeed();
}

// 验证瞄准 RPC 参数
// 流程：检查角色未死亡
bool AFPS_CharacterBase::Server_SetAiming_Validate(bool bNewAiming)
{
    return !bIsDead;
}

// 服务器执行瞄准状态同步
// 流程：更新 Aiming → 立即应用移动速度
void AFPS_CharacterBase::Server_SetAiming_Implementation(bool bNewAiming)
{
    Aiming = bNewAiming;
    ApplyMovementSpeed();
}

// 验证武器切换 RPC 参数
// 流程：始终返回 true
bool AFPS_CharacterBase::Server_RequestSwitchWeapon_Validate(AWeaponActor* NewWeapon)
{
    return true;
}

// 服务器执行武器切换
// 流程：调用 SwitchToWeapon 执行完整切换逻辑
void AFPS_CharacterBase::Server_RequestSwitchWeapon_Implementation(AWeaponActor* NewWeapon)
{
    SwitchToWeapon(NewWeapon);
}

// 服务器同步控制器旋转
// 流程：设置 Controller 的 ControlRotation 为客户端上报值
void AFPS_CharacterBase::Server_SetRotation_Implementation(FRotator NewRotation)
{
    if (Controller)
    {
        Controller->SetControlRotation(NewRotation);
    }
}

// 验证丢弃武器 RPC 参数
// 流程：始终返回 true
bool AFPS_CharacterBase::Server_RequestDrop_Validate(AWeaponActor* DroppedWeapon)
{
    return true;
}

// 服务器同步丢弃武器后的库存状态
// 流程：从 WeaponInventory 移除丢弃的武器 → 更新 CurrentWeapon 为库存第一把
void AFPS_CharacterBase::Server_RequestDrop_Implementation(AWeaponActor* DroppedWeapon)
{
    if (DroppedWeapon)
    {
        WeaponInventory.Remove(DroppedWeapon);
    }
    CurrentWeapon = WeaponInventory.Num() > 0 ? WeaponInventory[0] : nullptr;
}

// 根据当前状态应用对应的移动速度
// 流程：瞄准时用 AimSpeed → 冲刺时用 SprintSpeed → 否则用 WalkSpeed → 写入 MovementComponent.MaxWalkSpeed
void AFPS_CharacterBase::ApplyMovementSpeed()
{
    float TargetSpeed = WalkSpeed;

    if (Aiming)
        TargetSpeed = AimSpeed;
    else if (Sprinting)
        TargetSpeed = SprintSpeed;

    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->MaxWalkSpeed = TargetSpeed;
    }
}

// 获取射击起点（基类默认：Actor位置）
// 流程：返回 GetActorLocation
FVector AFPS_CharacterBase::GetShootLocation() const
{
    return GetActorLocation();
}

// 获取射击方向（基类默认：Actor旋转）
// 流程：返回 GetActorRotation
FRotator AFPS_CharacterBase::GetShootRotation() const
{
    return GetActorRotation();
}

// 获取第一人称动画网格（基类默认：第三人称Mesh）
// 流程：返回 GetMesh
USkeletalMeshComponent* AFPS_CharacterBase::GetFPAnimMesh() const
{
    return GetMesh();
}

// 更新脚步声计时器
// 流程：未着地或无音效时清除计时器 → 速度过低时暂停 → 否则首次启动时播放脚步声
void AFPS_CharacterBase::UpdateFootsteps()
{
    if (!FootstepSound || !bIsGrounded)
    {
        if (bFootstepActive)
        {
            GetWorldTimerManager().ClearTimer(FootstepTimerHandle);
            bFootstepActive = false;
        }
        return;
    }

    const float Speed2D = GetVelocity().Size2D();
    if (Speed2D < 10.f)
    {
        if (bFootstepActive)
        {
            GetWorldTimerManager().ClearTimer(FootstepTimerHandle);
            bFootstepActive = false;
        }
        return;
    }

    if (!bFootstepActive)
    {
        PlayFootstep();
    }
}

// 播放脚步声并设置下次触发定时器
// 流程：播放音效 → 根据冲刺/行走选择间隔 → 设置定时器递归调用自身
void AFPS_CharacterBase::PlayFootstep()
{
    if (!FootstepSound) return;

    UGameplayStatics::SpawnSoundAtLocation(this, FootstepSound, GetActorLocation());

    const float Interval = Sprinting ? FootstepIntervalSprint : FootstepIntervalWalk;
    GetWorldTimerManager().SetTimer(FootstepTimerHandle, this, &AFPS_CharacterBase::PlayFootstep, Interval, false);
    bFootstepActive = true;
}

// 装备武器Actor
// 流程：检查有效性 → 新武器：停用旧武器 → 调用 Weapon->Equip → 加入库存并激活 → 广播 OnWeaponActivated → 已拥有则切换
bool AFPS_CharacterBase::EquipWeaponActor(AWeaponActor* Weapon)
{
    if (!Weapon || CurrentWeapon == Weapon) return false;

    const bool bAlreadyOwned = WeaponInventory.Contains(Weapon);

    if (!bAlreadyOwned)
    {
        if (HasWeaponOfClass(Weapon->GetClass()))
            return false;

        if (WeaponInventory.Num() >= MaxWeaponCount)
            return false;

        AWeaponActor* PreviousWeapon = CurrentWeapon;
        if (CurrentWeapon)
            CurrentWeapon->DeactivateWeapon();

        CurrentWeapon = Weapon;
        if (!Weapon->Equip(this))
        {
            CurrentWeapon = PreviousWeapon;
            if (CurrentWeapon)
                CurrentWeapon->ActivateWeapon();
            return false;
        }

        WeaponInventory.Add(Weapon);
        Weapon->ActivateWeapon();
        OnWeaponActivated.Broadcast(Weapon);
        return true;
    }

    return SwitchToWeapon(Weapon);
}

// 检查是否可以将武器装备到指定槽位
// 流程：已拥有则允许 → 库存满则拒绝 → 已有同类型则拒绝 → 否则允许
bool AFPS_CharacterBase::CanEquipWeaponInSlot(EWeaponTargetSlot Slot, const AWeaponActor* Weapon) const
{
    if (!Weapon) return false;

    if (WeaponInventory.Contains(const_cast<AWeaponActor*>(Weapon)))
        return true;

    if (WeaponInventory.Num() >= MaxWeaponCount)
        return false;

    if (HasWeaponOfClass(Weapon->GetClass()))
        return false;

    return true;
}

// 当前武器弹药用尽时自动切换到备用武器
// 流程：检查是否为当前武器 → 查找有弹药的备用武器 → 切换
void AFPS_CharacterBase::HandleActiveWeaponOutOfAmmo(AWeaponActor* DepletedWeapon)
{
    if (!DepletedWeapon || DepletedWeapon != CurrentWeapon) return;

    AWeaponActor* Inactive = GetInactiveWeapon();
    if (Inactive && Inactive->HasAnyAmmo())
        SwitchToWeapon(Inactive);
}

// 切换到库存中的另一把武器
// 流程：库存不足2把或正换弹/检视时忽略 → 获取非活跃武器 → 本地切换 + 通知服务器
void AFPS_CharacterBase::SwitchWeapon()
{
    if (WeaponInventory.Num() < 2) return;
    if (Reloading || Viewing) return;

    AWeaponActor* NextWeapon = GetInactiveWeapon();
    if (NextWeapon)
    {
        SwitchToWeapon(NextWeapon);
        Server_RequestSwitchWeapon(NextWeapon);
    }
}

// 切换到指定武器
// 流程：检查有效性 → 停用旧武器 → 重置换弹/检视/瞄准状态 → 激活新武器 → 广播 OnWeaponActivated
bool AFPS_CharacterBase::SwitchToWeapon(AWeaponActor* Weapon)
{
    if (!Weapon) return false;
    if (CurrentWeapon == Weapon) return false;
    if (!WeaponInventory.Contains(Weapon)) return false;
    if (Reloading || Viewing) return false;

    if (CurrentWeapon)
        CurrentWeapon->DeactivateWeapon();

    Reloading = false;
    Viewing = false;
    Aiming = false;

    CurrentWeapon = Weapon;
    CurrentWeapon->ActivateWeapon();
    OnWeaponActivated.Broadcast(Weapon);
    return true;
}

// 丢弃当前武器
// 流程：库存仅1把或正换弹/检视时忽略 → 移除当前武器 → 调用 DropWeapon → 重新激活库存第一把 → 通知服务器
void AFPS_CharacterBase::DropCurrentWeapon()
{
    if (WeaponInventory.Num() <= 1) return;
    if (!CurrentWeapon) return;
    if (Reloading || Viewing) return;

    Reloading = false;
    Viewing = false;
    Aiming = false;

    AWeaponActor* WeaponToDrop = CurrentWeapon;
    WeaponInventory.Remove(WeaponToDrop);
    WeaponToDrop->DropWeapon();

    if (WeaponInventory.Num() > 0)
    {
        CurrentWeapon = WeaponInventory[0];
        CurrentWeapon->ActivateWeapon();
        OnWeaponActivated.Broadcast(CurrentWeapon);
    }
    else
    {
        CurrentWeapon = nullptr;
    }

    Server_RequestDrop(WeaponToDrop);
}

// 获取库存中非当前武器的第一把武器
// 流程：遍历 WeaponInventory → 返回第一个 != CurrentWeapon 的武器
AWeaponActor* AFPS_CharacterBase::GetInactiveWeapon() const
{
    for (AWeaponActor* W : WeaponInventory)
    {
        if (W && W != CurrentWeapon)
            return W;
    }
    return nullptr;
}

// 检查库存中是否已存在指定类型的武器
// 流程：遍历 WeaponInventory → 对比每个武器的 GetClass() 与 WeaponClass
bool AFPS_CharacterBase::HasWeaponOfClass(TSubclassOf<AWeaponActor> WeaponClass) const
{
    if (!WeaponClass) return false;
    for (const AWeaponActor* W : WeaponInventory)
    {
        if (W && W->GetClass() == WeaponClass)
            return true;
    }
    return false;
}
