#include "Character/FPS_CharacterBase.h"
#include "Weapon/WeaponActor.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

AFPS_CharacterBase::AFPS_CharacterBase()
{
    bReplicates = true;
}

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

void AFPS_CharacterBase::OnRep_CurrentHealth()
{
    // 客户端收到生命值更新后的回调
    // 可用于更新UI、播放受伤特效等
}

void AFPS_CharacterBase::OnRep_CurrentWeapon()
{
    // 远程客户端：根据新武器更新可见性
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

void AFPS_CharacterBase::OnRep_Sprinting()
{
    ApplyMovementSpeed();
}

void AFPS_CharacterBase::OnRep_Aiming()
{
    ApplyMovementSpeed();
}

void AFPS_CharacterBase::OnRep_Reloading()
{
    ApplyMovementSpeed();
}

void AFPS_CharacterBase::OnRep_Viewing()
{
    ApplyMovementSpeed();
}

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

void AFPS_CharacterBase::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        bIsGrounded = !MoveComp->IsFalling();
    }

    UpdateFootsteps();
}

float AFPS_CharacterBase::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
    AController* EventInstigator, AActor* DamageCauser)
{
    if (bIsDead) return 0.f;
    if (!HasAuthority()) return 0.f;  // 仅服务器执行伤害

    const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
    CurrentHealth = FMath::Max(CurrentHealth - ActualDamage, 0.f);  // 自动复制 → OnRep_CurrentHealth

    if (CurrentHealth <= 0.f)
    {
        bIsDead = true;
        HandleDeath();            // 服务器权威状态变更
        Multicast_OnDeath();      // 广播死亡表现给所有客户端
        OnDeath.Broadcast();      // 本地委托（蓝图可绑定）
    }

    return ActualDamage;
}

void AFPS_CharacterBase::HandleDeath()
{
    // 禁用碰撞
    if (UCapsuleComponent* Capsule = GetCapsuleComponent())
    {
        Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->DisableMovement();
    }

    // 先清除引用，防止 OnRep 回调访问已销毁武器
    AWeaponActor* PrevWeapon = CurrentWeapon;
    CurrentWeapon = nullptr;

    // 停用并销毁所有手持武器
    for (AWeaponActor* Weapon : WeaponInventory)
    {
        if (Weapon)
        {
            Weapon->DeactivateWeapon();
            Weapon->Destroy();
        }
    }
    WeaponInventory.Empty();
}

void AFPS_CharacterBase::Multicast_OnDeath_Implementation()
{
    // 基类默认：禁用碰撞和移动（服务器已执行，客户端同步执行确保一致）
    if (UCapsuleComponent* Capsule = GetCapsuleComponent())
    {
        Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->DisableMovement();
    }
}

// ---------- 拾取 Server RPC ----------
bool AFPS_CharacterBase::Server_RequestPickup_Validate(AWeaponActor* Weapon)
{
    return Weapon != nullptr;
}

void AFPS_CharacterBase::Server_RequestPickup_Implementation(AWeaponActor* Weapon)
{
    EquipWeaponActor(Weapon);
}

// ---------- 运动 (基类默认: 直接设置状态) ----------
void AFPS_CharacterBase::Sprint()
{
    if (Reloading) return;
    Sprinting = true;
    ApplyMovementSpeed();
}

void AFPS_CharacterBase::StopSprint()
{
    Sprinting = false;
    ApplyMovementSpeed();
}

void AFPS_CharacterBase::Aim()
{
    Aiming = true;
    ApplyMovementSpeed();
}

void AFPS_CharacterBase::StopAim()
{
    Aiming = false;
    ApplyMovementSpeed();
}

// ---------- 服务器 RPC ----------
bool AFPS_CharacterBase::Server_SetSprinting_Validate(bool bNewSprinting)
{
    return !bIsDead;
}

void AFPS_CharacterBase::Server_SetSprinting_Implementation(bool bNewSprinting)
{
    Sprinting = bNewSprinting;
    ApplyMovementSpeed(); // 立即同步速度，减少客户端预测修正
}

bool AFPS_CharacterBase::Server_SetAiming_Validate(bool bNewAiming)
{
    return !bIsDead;
}

void AFPS_CharacterBase::Server_SetAiming_Implementation(bool bNewAiming)
{
    Aiming = bNewAiming;
    ApplyMovementSpeed(); // 立即同步速度，减少客户端预测修正
}

// ---------- 武器操作 Server RPC (服务器只转发同步) ----------
bool AFPS_CharacterBase::Server_RequestSwitchWeapon_Validate(AWeaponActor* NewWeapon)
{
    return true;
}

void AFPS_CharacterBase::Server_RequestSwitchWeapon_Implementation(AWeaponActor* NewWeapon)
{
    // 服务器只同步 CurrentWeapon，不重新执行切换逻辑
    if (CurrentWeapon)
        CurrentWeapon->DeactivateWeapon();
    CurrentWeapon = NewWeapon;
}

bool AFPS_CharacterBase::Server_RequestDrop_Validate(AWeaponActor* DroppedWeapon)
{
    return true;
}

void AFPS_CharacterBase::Server_RequestDrop_Implementation(AWeaponActor* DroppedWeapon)
{
    // 服务器只同步 CurrentWeapon，不重新执行丢弃逻辑
    if (DroppedWeapon)
    {
        WeaponInventory.Remove(DroppedWeapon);
    }
    CurrentWeapon = WeaponInventory.Num() > 0 ? WeaponInventory[0] : nullptr;
}

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

// ---------- 多态接口默认实现 ----------
FVector AFPS_CharacterBase::GetShootLocation() const
{
    return GetActorLocation();
}

FRotator AFPS_CharacterBase::GetShootRotation() const
{
    return GetActorRotation();
}

USkeletalMeshComponent* AFPS_CharacterBase::GetFPAnimMesh() const
{
    return GetMesh();
}

// ---------- 脚步声 ----------
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

void AFPS_CharacterBase::PlayFootstep()
{
    if (!FootstepSound) return;

    UGameplayStatics::SpawnSoundAtLocation(this, FootstepSound, GetActorLocation());

    const float Interval = Sprinting ? FootstepIntervalSprint : FootstepIntervalWalk;
    GetWorldTimerManager().SetTimer(FootstepTimerHandle, this, &AFPS_CharacterBase::PlayFootstep, Interval, false);
    bFootstepActive = true;
}

// ---------- 武器接口 ----------
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

void AFPS_CharacterBase::HandleActiveWeaponOutOfAmmo(AWeaponActor* DepletedWeapon)
{
    if (!DepletedWeapon || DepletedWeapon != CurrentWeapon) return;

    AWeaponActor* Inactive = GetInactiveWeapon();
    if (Inactive && Inactive->HasAnyAmmo())
        SwitchToWeapon(Inactive);
}

void AFPS_CharacterBase::SwitchWeapon()
{
    if (WeaponInventory.Num() < 2) return;
    if (Reloading || Viewing) return;

    AWeaponActor* NextWeapon = GetInactiveWeapon();
    if (NextWeapon)
    {
        SwitchToWeapon(NextWeapon);        // 本地执行
        Server_RequestSwitchWeapon(NextWeapon);  // 通知服务器同步
    }
}

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

    Server_RequestDrop(WeaponToDrop);  // 通知服务器同步
}

AWeaponActor* AFPS_CharacterBase::GetInactiveWeapon() const
{
    for (AWeaponActor* W : WeaponInventory)
    {
        if (W && W != CurrentWeapon)
            return W;
    }
    return nullptr;
}

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
