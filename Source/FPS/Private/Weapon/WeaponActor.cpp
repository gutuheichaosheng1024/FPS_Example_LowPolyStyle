#include "Weapon/WeaponActor.h"
#include "Character/FPS_CharacterBase.h"
#include "Weapon/PickUpComponent.h"
#include "Weapon/WeaponAnimationConfig.h"
#include "Weapon/WeaponAudioConfig.h"
#include "Weapon/WeaponDataConfig.h"
#include "Weapon/WeaponRecoilHandler.h"

#include "Kismet/KismetMathLibrary.h"
#include "Animation/AnimInstance.h"
#include "Components/AudioComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Particles/ParticleSystem.h"
#include "Perception/AISense_Hearing.h"
#include "Net/UnrealNetwork.h"

AWeaponActor::AWeaponActor()
{
    bReplicates = true;
    PrimaryActorTick.bCanEverTick = true;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
    WeaponMeshStatic = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMeshStatic"));
    PickupComponent = CreateDefaultSubobject<UPickUpComponent>(TEXT("WeaponPickupComponent"));

    if (WeaponMesh)
        WeaponMesh->SetupAttachment(Root);
    if (WeaponMeshStatic)
        WeaponMeshStatic->SetupAttachment(Root);
    if (PickupComponent)
        PickupComponent->SetupAttachment(WeaponMeshStatic);
}

void AWeaponActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION(AWeaponActor, TotalAmmo, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(AWeaponActor, CurrentAmmo, COND_OwnerOnly);
}

void AWeaponActor::OnRep_CurrentAmmo()
{
}

// Owner 复制回调：将 AActor::Owner 桥接到 OwningCharacter，解决客户端 OwningCharacter 为 null 的问题
// 流程：调用父类 OnRep_Owner → 若 OwningCharacter 为空则将 GetOwner 转换为 AFPS_CharacterBase → 赋值给 OwningCharacter → 尝试绑定输入
void AWeaponActor::OnRep_Owner()
{
    Super::OnRep_Owner();

    if (!OwningCharacter)
    {
        if (AFPS_CharacterBase* Char = Cast<AFPS_CharacterBase>(GetOwner()))
        {
            OwningCharacter = Char;
            TryBindInput();
        }
    }
}

// BeginPlay：初始化网格管理器、弹药和自动销毁定时器
// 流程：缓存弹夹/瞄准插槽 → 设置静态组件 Owner 可见性 → 未装备时隐藏 FP Mesh 并应用掉落物理 → 从 DataConfig 初始化弹药 → 设置弹夹正常状态 → 未装备时禁用 Tick 并启动自动销毁
void AWeaponActor::BeginPlay()
{
    Super::BeginPlay();

    MeshManager.CacheMagazineSlots(WeaponMesh, WeaponMeshStatic);
    MeshManager.CacheAimSocket(WeaponMesh);
    MeshManager.SetStaticMeshComponentsOwnerVisibility(WeaponMesh, WeaponMeshStatic);

    if (!bIsEquipped && WeaponMeshStatic)
    {
        WeaponMesh->SetVisibility(false, true);
        ApplyDroppedPhysicsState();
    }

    if (DataConfig)
    {
        CurrentAmmo = DataConfig->MaxAmmo;
        TotalAmmo = DataConfig->DefaultMagazineNum * DataConfig->MaxAmmo;
    }

    MeshManager.SetMagazineNormalState();

    if (!bIsEquipped)
    {
        SetActorTickEnabled(false);
        ScheduleAutoDestroy();
    }
}

// EndPlay：清理输入绑定和所有定时器
// 流程：解绑输入 → 清空 OwningCharacter → 清除所有定时器（AutoFire/Reload/PickupReactivate/AutoDestroy）→ 停止音效 → 清空委托
void AWeaponActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    UnbindInput();
    OwningCharacter = nullptr;

    UWorld* World = GetWorld();
    if (World)
    {
        FTimerManager& TM = World->GetTimerManager();
        TM.ClearTimer(AutoFireTimerHandle);
        TM.ClearTimer(ReloadTimerHandle);
        TM.ClearTimer(PickupReactivateHandle);
        TM.ClearTimer(AutoDestroyTimerHandle);
    }

    StopActiveSound();

    OnWeaponReady.Clear();
    ReloadMontageEndedDelegate.Unbind();
    ViewMontageEndedDelegate.Unbind();
    FireMontageEndedDelegate.Unbind();

    Super::EndPlay(EndPlayReason);
}

// Tick：更新扩散和后坐力
// 流程：调用 SpreadHandler.UpdateSpread → 调用 RecoilHandler.UpdateRecoil
void AWeaponActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    if (DeltaTime > KINDA_SMALL_NUMBER && DataConfig)
    {
        SpreadHandler.UpdateSpread(DeltaTime, DataConfig);
    }
    if (DataConfig)
    {
        RecoilHandler.UpdateRecoil(DeltaTime, DataConfig);
    }
}

bool AWeaponActor::Equip(AFPS_CharacterBase* TargetCharacter)
{
    return HandleEquip(TargetCharacter);
}

// 装备武器到目标角色
// 流程：验证目标角色和武器槽可用性 → 设置 OwningCharacter 和 Owner → Mesh 挂载到角色对应插槽 → AttachToActor → 禁用拾取组件并关闭物理 → 初始化后坐力/扩散处理器 → 绑定输入
bool AWeaponActor::HandleEquip(AFPS_CharacterBase* TargetCharacter)
{
    if (!IsValid(TargetCharacter)) return false;

    if (!TargetCharacter->CanEquipWeaponInSlot(TargetSlot, this))
        return false;

    OwningCharacter = TargetCharacter;
    SetOwner(TargetCharacter);
    MeshManager.AttachToCharacter(OwningCharacter, WeaponMesh, WeaponMeshStatic, AttachSocketName);
    AttachToActor(TargetCharacter, FAttachmentTransformRules::SnapToTargetNotIncludingScale);

    HandlePickupComponentPostEquip();
    bIsEquipped = true;
    bIsActiveWeapon = false;

    RecoilHandler.Init(TargetCharacter);
    SpreadHandler.Init(TargetCharacter);
    BindInput();
    return true;
}

void AWeaponActor::ActivateWeapon()
{
    HandleActivateWeapon();
}

// 激活武器视觉效果：显示 Mesh、弹夹状态、设置 AnimBP、播放拔枪动画
// 流程：解锁武器操作 → 显示 FP/TP Mesh → 根据换弹状态设置弹夹可见性 → 尝试绑定输入 → 设置角色 FP/TP AnimBP → 播放拔枪蒙太奇
void AWeaponActor::HandleActivateWeaponVisuals()
{
    SetWeaponActionsLocked(false);
    MeshManager.ShowWeaponMeshes(true, WeaponMesh, WeaponMeshStatic);
    if (OwningCharacter && OwningCharacter->Reloading)
        MeshManager.SetMagazineReloadState();
    else
        MeshManager.SetMagazineNormalState();
    TryBindInput();

    if (OwningCharacter && AnimationConfig)
    {
        USkeletalMeshComponent* FPMesh = OwningCharacter->GetFPAnimMesh();
        USkeletalMeshComponent* TPMesh = OwningCharacter->GetMesh();
        const bool bHasDedicatedFPMesh = (FPMesh != nullptr && FPMesh != TPMesh);

        if (bHasDedicatedFPMesh && AnimationConfig->FP_CharacterAnimBlueprintClass)
        {
            FPMesh->SetAnimInstanceClass(AnimationConfig->FP_CharacterAnimBlueprintClass);
        }
        if (AnimationConfig->TP_CharacterAnimBlueprintClass)
        {
            TPMesh->SetAnimInstanceClass(AnimationConfig->TP_CharacterAnimBlueprintClass);
        }
    }

    Unholster();
}

// 激活武器状态
// 流程：设置 bIsEquipped/bIsActiveWeapon → 启用 Tick → 调用激活视觉效果
void AWeaponActor::HandleActivateWeapon()
{
    if (!OwningCharacter) return;

    bIsEquipped = true;
    bIsActiveWeapon = true;
    SetActorTickEnabled(true);

    HandleActivateWeaponVisuals();
}

void AWeaponActor::DeactivateWeapon()
{
    HandleDeactivateWeapon();
}

// 停用武器视觉效果：锁定操作、隐藏 Mesh、解绑输入、清除换弹状态
// 流程：锁定武器操作 → 未装备时显示 Mesh 并恢复弹夹 → 已装备时隐藏 Mesh 并隐藏弹夹 → 解绑输入 → 清除角色 Reloading 状态
void AWeaponActor::HandleDeactivateWeaponVisuals()
{
    SetWeaponActionsLocked(true);

    if (!bIsEquipped || !OwningCharacter)
    {
        MeshManager.ShowWeaponMeshes(true, WeaponMesh, WeaponMeshStatic);
        MeshManager.SetMagazineNormalState();
    }
    else
    {
        MeshManager.ShowWeaponMeshes(false, WeaponMesh, WeaponMeshStatic);
        MeshManager.HideAllMagazines();
    }

    UnbindInput();

    if (OwningCharacter && OwningCharacter->Reloading)
    {
        OwningCharacter->Reloading = false;
    }
}

// 停用武器状态
// 流程：设置 bIsActiveWeapon = false → 禁用 Tick → 调用停用视觉效果
void AWeaponActor::HandleDeactivateWeapon()
{
    bIsActiveWeapon = false;
    SetActorTickEnabled(false);
    HandleDeactivateWeaponVisuals();
}

void AWeaponActor::DropWeapon()
{
    HandleDropWeapon();
}

// 丢弃武器：停止射击、卸载 Mesh、应用物理、清理状态
// 流程：停止射击和音效 → 解绑输入 → 禁用 Tick → 从角色卸载 Mesh → DetachFromActor → 应用掉落物理（位置/旋转/冲量）→ 延迟 0.5s 启用拾取组件 → 清空状态和处理器
void AWeaponActor::HandleDropWeapon()
{
    if (!bIsEquipped || !OwningCharacter) return;

    FVector ViewLoc = OwningCharacter->GetShootLocation();
    FRotator ViewRot = OwningCharacter->GetShootRotation();

    HandleStopFire();
    StopActiveSound();
    UnbindInput();
    SetActorTickEnabled(false);

    MeshManager.DetachFromCharacter(OwningCharacter, WeaponMesh, WeaponMeshStatic);
    DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    DropWeaponPhysically(ViewLoc, ViewRot);

    GetWorldTimerManager().SetTimer(
        PickupReactivateHandle,
        [this]() { EnablePickupComponent(true); },
        0.5f,
        false);

    bIsEquipped = false;
    bIsActiveWeapon = false;
    RecoilHandler.ClearCharacter();
    SpreadHandler.ClearCharacter();
    OwningCharacter = nullptr;
    CachedInputComponent = nullptr;

    if (WeaponMesh)
        WeaponMesh->SetVisibility(false, true);
}

// 绑定 EnhancedInput 动作到武器回调
// 流程：获取 PlayerController → 获取 EnhancedInputComponent → 绑定 FireAction（Pressed/Released）、ReloadAction、ViewAction → 缓存绑定指针
void AWeaponActor::BindInput()
{
    if (!OwningCharacter || !bIsActiveWeapon) return;

    APlayerController* PC = Cast<APlayerController>(OwningCharacter->GetController());
    if (!PC) return;

    if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PC->InputComponent))
    {
        CachedInputComponent = EnhancedInput;

        if (FireAction)
        {
            FirePressedBinding = &EnhancedInput->BindAction(FireAction, ETriggerEvent::Started, this, &AWeaponActor::OnFirePressed);
            FireReleasedBinding = &EnhancedInput->BindAction(FireAction, ETriggerEvent::Completed, this, &AWeaponActor::OnFireReleased);
        }
        if (ReloadAction)
        {
            ReloadBinding = &EnhancedInput->BindAction(ReloadAction, ETriggerEvent::Started, this, &AWeaponActor::Reload);
        }
        if (ViewAction)
        {
            ViewBinding = &EnhancedInput->BindAction(ViewAction, ETriggerEvent::Started, this, &AWeaponActor::View);
        }
    }
}

// 解绑所有 EnhancedInput 动作
// 流程：依次移除 FirePressed/FireReleased/Reload/View 绑定 → 清空绑定指针和缓存组件
void AWeaponActor::UnbindInput()
{
    if (!CachedInputComponent) return;

    if (FirePressedBinding)
    {
        CachedInputComponent->RemoveBinding(*FirePressedBinding);
        FirePressedBinding = nullptr;
    }
    if (FireReleasedBinding)
    {
        CachedInputComponent->RemoveBinding(*FireReleasedBinding);
        FireReleasedBinding = nullptr;
    }
    if (ReloadBinding)
    {
        CachedInputComponent->RemoveBinding(*ReloadBinding);
        ReloadBinding = nullptr;
    }
    if (ViewBinding)
    {
        CachedInputComponent->RemoveBinding(*ViewBinding);
        ViewBinding = nullptr;
    }
    CachedInputComponent = nullptr;
}

// 尝试绑定输入（用于 OnRep_Owner 延迟绑定场景）
// 流程：若 CachedInputComponent 为空且武器已激活且有 OwningCharacter → 调用 BindInput
void AWeaponActor::TryBindInput()
{
    if (!CachedInputComponent && bIsActiveWeapon && OwningCharacter)
        BindInput();
}

// 输入按下回调：设置按住标记并开火
// 流程：设置 bFireInputHeld = true → 调用 Fire()
void AWeaponActor::OnFirePressed()
{
    bFireInputHeld = true;
    Fire();
}

// 输入释放回调：清除按住标记并停火
// 流程：设置 bFireInputHeld = false → 调用 StopFire()
void AWeaponActor::OnFireReleased()
{
    bFireInputHeld = false;
    StopFire();
}

// 拔枪动画：清除换弹状态，播放 FP 角色+武器蒙太奇和音效
// 流程：清除角色 Reloading 状态 → 播放 C_Unholster + W_Unholster 蒙太奇 → 播放拔枪音效
void AWeaponActor::Unholster()
{
    if (!OwningCharacter || !AnimationConfig) return;
    if (OwningCharacter->Reloading)
    {
        OwningCharacter->Reloading = false;
    }
    PlayFPCharacterAndWeaponMontage(AnimationConfig->C_UnholsterAnimation, AnimationConfig->W_UnholsterAnimation);
    if (AudioConfig) PlaySound(AudioConfig->UnholsterSound);
}

void AWeaponActor::Fire()
{
    HandleStartFire();
}

void AWeaponActor::StopFire()
{
    HandleStopFire();
}

// 换弹请求：客户端预测播放动画+音效，通知服务器执行权威逻辑
// 流程：检查前置条件（换弹中/冲刺中/弹药满）→ 重置后坐力 → 设置角色 Reloading 状态并清除 Aiming → 设置弹夹换弹状态 → 本地播放 FP 动画+音效 → 客户端发 Server RPC / 服务器直接走 HandleReloadRequest
void AWeaponActor::Reload()
{
    if (!OwningCharacter || !DataConfig) return;
    if (OwningCharacter->Reloading || OwningCharacter->Sprinting) return;
    if (CurrentAmmo >= DataConfig->MaxAmmo || TotalAmmo <= 0) return;

    ResetRecoilPattern();
    OwningCharacter->Reloading = true;
    OwningCharacter->Aiming = false;
    OwningCharacter->ApplyMovementSpeed();

    MeshManager.SetMagazineReloadState();

    const bool bEmptyReload = (CurrentAmmo == 0);
    if (AnimationConfig)
    {
        UAnimMontage* C_Montage = (bEmptyReload && AnimationConfig->C_ReloadEmptyAnimation) ? AnimationConfig->C_ReloadEmptyAnimation : AnimationConfig->C_ReloadAnimation;
        UAnimMontage* W_Montage = (bEmptyReload && AnimationConfig->W_ReloadEmptyAnimation) ? AnimationConfig->W_ReloadEmptyAnimation : AnimationConfig->W_ReloadAnimation;
        PlayFPCharacterAndWeaponMontage(C_Montage, W_Montage);
    }
    USoundBase* ReloadSfx = nullptr;
    if (AudioConfig) ReloadSfx = (bEmptyReload && AudioConfig->ReloadEmptySound) ? AudioConfig->ReloadEmptySound : AudioConfig->ReloadSound;
    PlaySound(ReloadSfx);

    if (!OwningCharacter->HasAuthority())
    {
        Server_RequestReload();
    }
    else
    {
        HandleReloadRequest();
    }
}

// 检视武器：播放检视动画+音效，绑定蒙太奇结束委托，通知服务器
// 流程：检查前置条件 → 设置角色 Viewing 状态并清除 Aiming → 播放 FP 检视动画+音效 → 绑定 OnViewMontageEnded 委托 → 无动画则直接完成 → 客户端发 Server RPC
void AWeaponActor::View()
{
    if (bWeaponActionsLocked || !OwningCharacter || !OwningCharacter->GetController() || !bIsActiveWeapon || !AnimationConfig)
        return;

    bFireInputHeld = false;
    OwningCharacter->Viewing = true;
    OwningCharacter->Aiming = false;

    PlayFPCharacterAndWeaponMontage(AnimationConfig->C_ViewAnimation, nullptr);
    if (AudioConfig) PlaySound(AudioConfig->ViewSound);

    bool bDelegateBound = false;
    if (AnimationConfig->C_ViewAnimation)
    {
        if (USkeletalMeshComponent* Mesh1P = OwningCharacter->GetFPAnimMesh())
        {
            if (UAnimInstance* Anim = Mesh1P->GetAnimInstance())
            {
                ViewMontageEndedDelegate.Unbind();
                ViewMontageEndedDelegate.BindUObject(this, &AWeaponActor::OnViewMontageEnded);
                Anim->Montage_SetEndDelegate(ViewMontageEndedDelegate, AnimationConfig->C_ViewAnimation);
                bDelegateBound = true;
            }
        }
    }
    if (!bDelegateBound)
        HandleViewFinished();

    if (!OwningCharacter->HasAuthority())
        Server_RequestView(true);
}

// 停止检视：中断检视动画并清理状态
// 流程：检查检视动画是否在播放 → 停止所有蒙太奇并清理音效 → 调用 HandleViewFinished → 客户端发 Server RPC 同步状态
void AWeaponActor::StopView()
{
    if (!OwningCharacter || !AnimationConfig) return;
    UAnimInstance* CharAnim = OwningCharacter->GetFPAnimMesh() ? OwningCharacter->GetFPAnimMesh()->GetAnimInstance() : nullptr;
    if (CharAnim && AnimationConfig->C_ViewAnimation && CharAnim->Montage_IsPlaying(AnimationConfig->C_ViewAnimation))
    {
        CharAnim->StopAllMontages(0.2f);
        StopActiveSound();
        HandleViewFinished();
    }

    if (!OwningCharacter->HasAuthority())
        Server_RequestView(false);
}

// 开始射击：检查弹药和冷却，执行射击或触发换弹/空仓反馈
// 流程：检查前置条件（锁定/换弹/冲刺）→ 无弹药时尝试换弹或播放空仓音效 → 检查射击间隔冷却 → 调用 PerformFire_Local → 全自动模式启动 AutoFireTimer
void AWeaponActor::HandleStartFire()
{
    if (!OwningCharacter || bWeaponActionsLocked || !bIsActiveWeapon || !DataConfig) return;
    if (OwningCharacter->Reloading || OwningCharacter->Sprinting) return;
    if (CurrentAmmo <= 0)
    {
        if (TotalAmmo > 0)
            HandleReloadRequest();
        else
        {
            if (AudioConfig) PlaySound(AudioConfig->DryFireSound);
            NotifyOwnerOutOfAmmo();
        }
        return;
    }

    const float TimeSinceLastShot = GetWorld()->GetTimeSeconds() - LastFireTime;
    if (TimeSinceLastShot >= DataConfig->FireInterval * 0.8f)
    {
        PerformFire_Local();
    }

    if (DataConfig->FireMode == EWeaponFireMode::FullAuto && !GetWorldTimerManager().IsTimerActive(AutoFireTimerHandle))
    {
        GetWorldTimerManager().SetTimer(AutoFireTimerHandle, this, &AWeaponActor::HandleAutoFire, DataConfig->FireInterval, true, DataConfig->FireInterval);
    }
}

// 停止射击：清除全自动射击定时器
// 流程：检查 AutoFireTimer 是否活跃 → 是则清除
void AWeaponActor::HandleStopFire()
{
    if (GetWorldTimerManager().IsTimerActive(AutoFireTimerHandle))
        GetWorldTimerManager().ClearTimer(AutoFireTimerHandle);
}

// 全自动射击定时器回调：检查弹药和状态，执行射击或触发换弹/停火
// 流程：检查前置条件 → 无弹药时换弹或空仓停火 → 有弹药则调用 PerformFire_Local
void AWeaponActor::HandleAutoFire()
{
    if (!OwningCharacter || OwningCharacter->Reloading || OwningCharacter->Sprinting || !DataConfig) return;

    if (CurrentAmmo <= 0)
    {
        if (TotalAmmo > 0)
            HandleReloadRequest();
        else
        {
            if (AudioConfig) PlaySound(AudioConfig->DryFireSound);
            NotifyOwnerOutOfAmmo();
            HandleStopFire();
        }
        return;
    }
    PerformFire_Local();
}

// 本地执行射击：计算扩散方向，服务器执行权威逻辑，客户端本地预测表现
// 流程：记录射击时间 → 计算带扩散的射击方向 → 服务器：本地 LineTrace 预测 VFX + 调用权威射击 RPC → 客户端：本地扣弹药 + LineTrace 预测 VFX + 播放动画/音效/后坐力 + 发送 Server_RequestFire
void AWeaponActor::PerformFire_Local()
{
    if (!OwningCharacter) return;

    LastFireTime = GetWorld()->GetTimeSeconds();

    const FVector Forward = OwningCharacter->GetShootRotation().Vector();
    FVector ShotDir = Forward;
    float Spread = SpreadHandler.GetTotalSpreadAngle(DataConfig);
    if (Spread > KINDA_SMALL_NUMBER)
    {
        ShotDir = UKismetMathLibrary::RandomUnitVectorInConeInRadians(Forward, FMath::DegreesToRadians(Spread));
    }

    if (OwningCharacter->HasAuthority())
    {
        FHitResult AuthorityHit;
        FCollisionQueryParams AuthorityParams;
        AuthorityParams.AddIgnoredActor(OwningCharacter);
        AuthorityParams.AddIgnoredActor(this);
        const FVector AuthorityStart = OwningCharacter->GetShootLocation();
        const bool bAuthorityHit = GetWorld()->LineTraceSingleByChannel(AuthorityHit, AuthorityStart, AuthorityStart + ShotDir * 10000.f, ECC_GameTraceChannel1, AuthorityParams);
        const FVector AuthorityImpactPoint = bAuthorityHit ? AuthorityHit.ImpactPoint : (AuthorityStart + ShotDir * 5000.f);

        const bool bIsLocalPlayer = OwningCharacter->IsPlayerControlled() && OwningCharacter->IsLocallyControlled();
        if (bIsLocalPlayer)
        {
            const bool bWasAiming = OwningCharacter->Aiming;
            if (AnimationConfig)
            {
                PlayFPCharacterAndWeaponMontage(bWasAiming ? AnimationConfig->C_FireAimedAnimation : AnimationConfig->C_FireAnimation, AnimationConfig->W_FireAnimation);
            }
            if (AudioConfig) PlaySound(AudioConfig->FireSound);
            HandleFireVFX(AuthorityImpactPoint);
            RecoilHandler.ApplyRecoil(DataConfig);
            SpreadHandler.AddSpreadOnFire(DataConfig);
        }
        else if (!OwningCharacter->IsPlayerControlled())
        {
            if (AnimationConfig)
                PlayTPCharacterMontage(AnimationConfig->TP_FireAnimation);
            if (AudioConfig) PlaySound(AudioConfig->FireSound);
            HandleFireVFX(AuthorityImpactPoint);
        }

        Server_RequestFire_Implementation(OwningCharacter->GetShootLocation(), ShotDir);
        HandleFireFinished();
        return;
    }

    CurrentAmmo = FMath::Max(CurrentAmmo - 1, 0);

    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(OwningCharacter);
    Params.AddIgnoredActor(this);
    const FVector Start = OwningCharacter->GetShootLocation();
    const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, Start + ShotDir * 10000.f, ECC_GameTraceChannel1, Params);
    const FVector ImpactPoint = bHit ? Hit.ImpactPoint : (Start + ShotDir * 5000.f);

    const bool bWasAiming = OwningCharacter->Aiming;
    if (AnimationConfig)
    {
        PlayFPCharacterAndWeaponMontage(bWasAiming ? AnimationConfig->C_FireAimedAnimation : AnimationConfig->C_FireAnimation, AnimationConfig->W_FireAnimation);
    }
    if (AudioConfig) PlaySound(AudioConfig->FireSound);
    HandleFireVFX(ImpactPoint);
    SpreadHandler.AddSpreadOnFire(DataConfig);
    RecoilHandler.ApplyRecoil(DataConfig);

    Server_RequestFire(Start, ShotDir);

    HandleFireFinished();
}

bool AWeaponActor::Server_RequestFire_Validate(FVector ShootLocation, FVector ShotDirection)
{
    return OwningCharacter != nullptr && !OwningCharacter->IsDead();
}

// Server RPC：服务器权威射击逻辑
// 流程：权威扣弹药 → 权威射线检测 → 命中时调用 ApplyDamage → 命中角色时广播 OnHitConfirmed → 报告 AI 听觉噪音 → Multicast 广播表现给远程客户端
void AWeaponActor::Server_RequestFire_Implementation(FVector ShootLocation, FVector ShotDirection)
{
    if (!OwningCharacter || !DataConfig) return;

    CurrentAmmo = FMath::Max(CurrentAmmo - 1, 0);

    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(OwningCharacter);
    Params.AddIgnoredActor(this);
    const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, ShootLocation, ShootLocation + ShotDirection * 10000.f, ECC_GameTraceChannel1, Params);

    if (bHit)
    {
        ApplyDamage(Hit, ShotDirection);

        if (AFPS_CharacterBase* HitChar = Cast<AFPS_CharacterBase>(Hit.GetActor()))
        {
            OnHitConfirmed.Broadcast(HitChar->IsDead());
        }
    }

    UAISense_Hearing::ReportNoiseEvent(GetWorld(), OwningCharacter->GetActorLocation(),
        1.0f, OwningCharacter, 2500.0f, TEXT("Gunshot"));

    Multicast_OnFire(bHit ? Hit.ImpactPoint : (ShootLocation + ShotDirection * 5000.f), bHit);
}

// Multicast RPC：远程客户端播放射击表现
// 流程：跳过本地射击者（已本地预测）→ 远程客户端播放 TP 蒙太奇 + 枪声 + VFX
void AWeaponActor::Multicast_OnFire_Implementation(FVector ImpactPoint, bool bHit)
{
    if (OwningCharacter && OwningCharacter->IsLocallyControlled()) return;

    if (AnimationConfig)
        PlayTPCharacterMontage(AnimationConfig->TP_FireAnimation);
    if (AudioConfig) PlaySound(AudioConfig->FireSound);
    HandleFireVFX(ImpactPoint);
}

bool AWeaponActor::Server_RequestReload_Validate()
{
    return OwningCharacter != nullptr && !OwningCharacter->IsDead();
}

// Server RPC：转发换弹请求到权威逻辑
// 流程：调用 HandleReloadRequest
void AWeaponActor::Server_RequestReload_Implementation()
{
    HandleReloadRequest();
}

// Multicast RPC：所有客户端恢复弹夹正常可见性
// 流程：调用 MeshManager.SetMagazineNormalState
void AWeaponActor::Multicast_OnReloadComplete_Implementation()
{
    MeshManager.SetMagazineNormalState();
}

bool AWeaponActor::Server_RequestView_Validate(bool bNewViewing)
{
    return OwningCharacter != nullptr;
}

// Server RPC：服务器同步检视状态
// 流程：设置角色 Viewing 状态 → 开始检视时清除 Aiming 状态
void AWeaponActor::Server_RequestView_Implementation(bool bNewViewing)
{
    OwningCharacter->Viewing = bNewViewing;
    if (bNewViewing)
        OwningCharacter->Aiming = false;
}

// 执行射线检测（含扩散），用于暴露给外部调用
// 流程：计算射击起点和带扩散的方向 → 设置碰撞参数忽略持有者和自身 → LineTraceSingleByChannel
bool AWeaponActor::PerformLineTrace(FHitResult& OutHit, FVector& OutShotDirection) const
{
    if (!OwningCharacter) return false;

    const FVector Start = OwningCharacter->GetShootLocation();
    const FVector Forward = OwningCharacter->GetShootRotation().Vector();
    FVector ShotDir = Forward;

    float Spread = SpreadHandler.GetTotalSpreadAngle(DataConfig);
    if (Spread > KINDA_SMALL_NUMBER)
    {
        ShotDir = UKismetMathLibrary::RandomUnitVectorInConeInRadians(Forward, FMath::DegreesToRadians(Spread));
    }
    OutShotDirection = ShotDir;
    FVector End = Start + ShotDir * 10000.f;

    FCollisionQueryParams Params;
    Params.AddIgnoredActor(OwningCharacter);
    Params.AddIgnoredActor(this);

    return GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, ECC_GameTraceChannel1, Params);
}

// 对命中结果施加物理冲量和伤害
// 流程：对物理模拟组件施加冲量 → 计算带浮动的伤害值 → 调用 ApplyPointDamage
void AWeaponActor::ApplyDamage(const FHitResult& Hit, const FVector& ShotDirection)
{
    if (!DataConfig) return;

    if (UPrimitiveComponent* HitComp = Hit.GetComponent())
    {
        if (HitComp->IsSimulatingPhysics())
        {
            HitComp->AddImpulseAtLocation(ShotDirection.GetSafeNormal() * 20000.f, Hit.ImpactPoint);
        }
    }

    if (AActor* HitActor = Hit.GetActor())
    {
        float FinalDamage = FMath::Max(DataConfig->BaseDamage + FMath::FRandRange(-DataConfig->DamageVariance, DataConfig->DamageVariance), 0.f);
        UGameplayStatics::ApplyPointDamage(HitActor, FinalDamage, ShotDirection.GetSafeNormal(), Hit,
            OwningCharacter ? OwningCharacter->GetController() : nullptr, this, UDamageType::StaticClass());
    }
}

// 通知持有者武器弹药耗尽
// 流程：调用 OwningCharacter->HandleActiveWeaponOutOfAmmo
void AWeaponActor::NotifyOwnerOutOfAmmo()
{
    if (OwningCharacter)
        OwningCharacter->HandleActiveWeaponOutOfAmmo(this);
}

// 服务器换弹请求：设置状态、播放 TP 动画、启动换弹完成定时器
// 流程：检查前置条件 → 设置角色 Reloading 状态并清除 Aiming → 设置弹夹换弹状态 → 播放 TP 换弹蒙太奇 → 以 FP 动画时长为基准启动定时器等待 HandleReloadComplete
void AWeaponActor::HandleReloadRequest()
{
    if (bWeaponActionsLocked || !OwningCharacter || !DataConfig) return;

    OwningCharacter->Reloading = true;
    OwningCharacter->Aiming = false;
    OwningCharacter->ApplyMovementSpeed();

    MeshManager.SetMagazineReloadState();

    const bool bEmptyReload = (CurrentAmmo == 0);
    if (AnimationConfig)
    {
        UAnimMontage* TP_Montage = (bEmptyReload && AnimationConfig->TP_ReloadEmptyAnimation) ? AnimationConfig->TP_ReloadEmptyAnimation : AnimationConfig->TP_ReloadAnimation;
        PlayTPCharacterMontage(TP_Montage);
    }

    UAnimMontage* C_Montage = nullptr;
    if (AnimationConfig)
        C_Montage = (bEmptyReload && AnimationConfig->C_ReloadEmptyAnimation) ? AnimationConfig->C_ReloadEmptyAnimation : AnimationConfig->C_ReloadAnimation;
    float Duration = C_Montage ? C_Montage->GetPlayLength() : 2.0f;
    GetWorldTimerManager().SetTimer(ReloadTimerHandle, this, &AWeaponActor::HandleReloadComplete, Duration, false);
}

// 换弹完成：补充弹药、清除状态、广播通知
// 流程：清除定时器 → 权威补充弹药 → 清除角色 Reloading 状态 → 恢复弹夹可见性 → Multicast 通知所有客户端 → 广播 OnWeaponReady
void AWeaponActor::HandleReloadComplete()
{
    GetWorldTimerManager().ClearTimer(ReloadTimerHandle);
    if (!OwningCharacter) return;

    RefillAmmo();
    OwningCharacter->Reloading = false;
    OwningCharacter->ApplyMovementSpeed();

    MeshManager.SetMagazineNormalState();

    Multicast_OnReloadComplete();

    OnWeaponReady.Broadcast();
}

// 补充弹药：从备弹转移到弹匣
// 流程：计算所需弹药 = MaxAmmo - CurrentAmmo → 转移量 = min(所需, TotalAmmo) → TotalAmmo -= 转移量 → CurrentAmmo += 转移量
void AWeaponActor::RefillAmmo()
{
    int Needed = DataConfig->MaxAmmo - CurrentAmmo;
    int Transfer = FMath::Min(Needed, TotalAmmo);
    TotalAmmo -= Transfer;
    CurrentAmmo += Transfer;
}

// 同时播放 FP 角色和武器蒙太奇，根据蒙太奇类型绑定结束委托
// 流程：获取角色 FP AnimInstance 和武器 AnimInstance → 播放角色蒙太奇 → 根据蒙太奇类型绑定 Fire/Reload 结束委托 → 播放武器蒙太奇 → 返回最大时长
float AWeaponActor::PlayFPCharacterAndWeaponMontage(UAnimMontage* CharMontage, UAnimMontage* WeaponMontage)
{
    if (!OwningCharacter || !bIsEquipped) return 0.f;

    UAnimInstance* CharAnim = OwningCharacter->GetFPAnimMesh() ? OwningCharacter->GetFPAnimMesh()->GetAnimInstance() : nullptr;
    UAnimInstance* WeaponAnim = WeaponMesh ? WeaponMesh->GetAnimInstance() : nullptr;
    float MaxDuration = 0.f;

    bool bPlayChar = CharMontage && CharAnim;
    bool bPlayWeapon = WeaponMontage && WeaponAnim;

    if (!bPlayChar && !bPlayWeapon) return 0.f;

    if (bPlayChar)
    {
        float Played = CharAnim->Montage_Play(CharMontage);
        MaxDuration = FMath::Max(MaxDuration, Played);
        if (AnimationConfig && (CharMontage == AnimationConfig->C_FireAnimation || CharMontage == AnimationConfig->C_FireAimedAnimation))
        {
            FireMontageEndedDelegate.Unbind();
            FireMontageEndedDelegate.BindUObject(this, &AWeaponActor::OnFireMontageEnded);
            CharAnim->Montage_SetEndDelegate(FireMontageEndedDelegate, CharMontage);
        }
        else if (AnimationConfig && (CharMontage == AnimationConfig->C_ReloadAnimation || CharMontage == AnimationConfig->C_ReloadEmptyAnimation))
        {
            ReloadMontageEndedDelegate.Unbind();
            ReloadMontageEndedDelegate.BindUObject(this, &AWeaponActor::OnReloadMontageEnded);
            CharAnim->Montage_SetEndDelegate(ReloadMontageEndedDelegate, CharMontage);
        }
        if (!bPlayWeapon && WeaponAnim)
            WeaponAnim->StopAllMontages(0.2f);
    }

    if (bPlayWeapon)
    {
        float Played = WeaponAnim->Montage_Play(WeaponMontage);
        MaxDuration = FMath::Max(MaxDuration, Played);
    }

    return MaxDuration;
}

// 播放第三人称角色蒙太奇
// 流程：获取角色 TP Mesh → 获取 AnimInstance → Montage_Play
void AWeaponActor::PlayTPCharacterMontage(UAnimMontage* CharMontage)
{
    if (!OwningCharacter || !CharMontage) return;
    if (USkeletalMeshComponent* Mesh = OwningCharacter->GetMesh())
    {
        if (UAnimInstance* Anim = Mesh->GetAnimInstance())
            Anim->Montage_Play(CharMontage);
    }
}

// 射击蒙太奇结束回调
// 流程：调用 HandleFireFinished
void AWeaponActor::OnFireMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    HandleFireFinished();
}

// 换弹蒙太奇结束回调（可能被中断，不做处理，真正的完成在定时器）
// 流程：无操作
void AWeaponActor::OnReloadMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
}

// 检视蒙太奇结束回调
// 流程：调用 HandleViewFinished
void AWeaponActor::OnViewMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    HandleViewFinished();
}

// 射击完成处理：允许冲刺并广播武器就绪
// 流程：设置角色 CanSprint = true → 广播 OnWeaponReady
void AWeaponActor::HandleFireFinished()
{
    if (OwningCharacter)
        OwningCharacter->CanSprint = true;
    OnWeaponReady.Broadcast();
}

// 换弹完成处理：清除角色换弹状态，恢复弹夹可见性
// 流程：设置角色 Reloading = false → 恢复弹夹正常状态
void AWeaponActor::HandleReloadFinished()
{
    if (OwningCharacter)
        OwningCharacter->Reloading = false;
    MeshManager.SetMagazineNormalState();
}

// 检视完成处理：清除角色检视状态
// 流程：设置角色 Viewing = false
void AWeaponActor::HandleViewFinished()
{
    if (OwningCharacter)
        OwningCharacter->Viewing = false;
}

// 在角色位置生成音效
// 流程：检查 Sound 和 OwningCharacter → 处理跳过标记 → 停止当前音效 → 生成新音效并缓存
void AWeaponActor::PlaySound(USoundBase* Sound)
{
    if (!Sound || !OwningCharacter) return;
    if (bSkipNextSound)
    {
        bSkipNextSound = false;
        return;
    }
    StopActiveSound();
    ActiveAudioComponent = UGameplayStatics::SpawnSoundAtLocation(this, Sound, OwningCharacter->GetActorLocation());
}

// 停止当前活跃音效
// 流程：若 ActiveAudioComponent 有效则调用 Stop → 清空指针
void AWeaponActor::StopActiveSound()
{
    if (ActiveAudioComponent.IsValid())
    {
        ActiveAudioComponent->Stop();
    }
    ActiveAudioComponent = nullptr;
}

// 在枪口发射点生成射击粒子特效，传入命中点局部坐标
// 流程：获取 SOCKET_EmitPoint 插槽变换 → SpawnEmitterAttached → 设置 HitPointLocation 参数
void AWeaponActor::HandleFireVFX(const FVector& HitPoint)
{
    if (!FireVFX || !OwningCharacter) return;
    USkeletalMeshComponent* AttachComp = WeaponMesh;
    if (!AttachComp) return;

    FTransform SocketTransform = AttachComp->GetSocketTransform(TEXT("SOCKET_EmitPoint"), RTS_World);
    UParticleSystemComponent* PSC = UGameplayStatics::SpawnEmitterAttached(
        FireVFX, AttachComp, TEXT("SOCKET_EmitPoint"),
        FVector::ZeroVector, FRotator::ZeroRotator,
        EAttachLocation::SnapToTargetIncludingScale, true);
    if (PSC)
    {
        FVector HitLocal = HitPoint.IsNearlyZero() ? FVector(0, 1, 0) : SocketTransform.InverseTransformPosition(HitPoint);
        PSC->SetVectorParameter(TEXT("HitPointLocation"), HitLocal);
    }
}

// 装备后禁用拾取组件物理和碰撞
// 流程：禁用 WeaponMeshStatic 物理模拟和碰撞 → 禁用 PickupComponent
void AWeaponActor::HandlePickupComponentPostEquip()
{
    if (WeaponMeshStatic)
    {
        WeaponMeshStatic->SetSimulatePhysics(false);
        WeaponMeshStatic->SetCollisionProfileName(TEXT("NoCollision"));
    }
    EnablePickupComponent(false);
}

// 启用/禁用拾取组件碰撞
// 流程：设置 bPickupComponentDisabled 标记 → 启用时设为 QueryOnly 并开启 Overlap → 禁用时设为 NoCollision
void AWeaponActor::EnablePickupComponent(bool bEnable)
{
    if (!PickupComponent) return;
    bPickupComponentDisabled = !bEnable;
    if (bEnable)
    {
        PickupComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        PickupComponent->SetGenerateOverlapEvents(true);
    }
    else
    {
        PickupComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        PickupComponent->SetGenerateOverlapEvents(false);
    }
}

// 应用掉落物理状态到 WeaponMeshStatic
// 流程：设置碰撞为 QueryAndPhysics → WorldDynamic 类型 → 阻挡所有通道忽略 Pawn → 有物理资产则启用物理模拟
void AWeaponActor::ApplyDroppedPhysicsState()
{
    if (!WeaponMeshStatic) return;
    WeaponMeshStatic->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    WeaponMeshStatic->SetCollisionObjectType(ECC_WorldDynamic);
    WeaponMeshStatic->SetCollisionResponseToAllChannels(ECR_Block);
    WeaponMeshStatic->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
    const bool bHasPhysicsAsset = WeaponMeshStatic->GetSkeletalMeshAsset()
        && WeaponMeshStatic->GetSkeletalMeshAsset()->GetPhysicsAsset();
    if (bHasPhysicsAsset)
    {
        WeaponMeshStatic->SetSimulatePhysics(true);
    }
}

// 将武器物理丢弃到屏幕前方
// 流程：应用掉落物理状态 → 设置位置为视野前方偏上 → 设置朝前旋转 → 施加线速度和随机角速度冲量
void AWeaponActor::DropWeaponPhysically(const FVector& ViewLoc, const FRotator& ViewRot)
{
    ApplyDroppedPhysicsState();
    SetActorLocation(ViewLoc + ViewRot.Vector() * 60.f + FVector::UpVector * 20.f);
    SetActorRotation(ViewRot);

    if (WeaponMeshStatic->IsSimulatingPhysics())
    {
        WeaponMeshStatic->SetPhysicsLinearVelocity(FVector::ZeroVector);
        WeaponMeshStatic->AddImpulse(ViewRot.Vector() * 400.f + FVector::UpVector * 200.f, NAME_None, true);
        WeaponMeshStatic->AddAngularImpulseInDegrees(
            FVector(
                FMath::FRandRange(-200.0f, 200.0f),
                FMath::FRandRange(-200.0f, 200.0f),
                FMath::FRandRange(-200.0f, 200.0f)
            ),
            NAME_None,
            true
        );
    }
}

// 启动自动销毁定时器（未拾取武器超时销毁）
// 流程：若已装备则跳过 → UnpickedLifeSpan <= 0 时直接销毁 → 否则设置定时器到期后 HandleAutoDestroyTimerExpired
void AWeaponActor::ScheduleAutoDestroy()
{
    if (bIsEquipped) return;
    if (UnpickedLifeSpan <= 0.f)
    {
        Destroy();
        return;
    }
    GetWorldTimerManager().SetTimer(AutoDestroyTimerHandle, this, &AWeaponActor::HandleAutoDestroyTimerExpired, UnpickedLifeSpan, false);
}

// 取消自动销毁定时器
// 流程：清除 AutoDestroyTimerHandle
void AWeaponActor::CancelAutoDestroy()
{
    GetWorldTimerManager().ClearTimer(AutoDestroyTimerHandle);
}

// 自动销毁定时器到期：未装备则销毁
// 流程：检查 bIsEquipped → 未装备则调用 Destroy()
void AWeaponActor::HandleAutoDestroyTimerExpired()
{
    if (!bIsEquipped)
        Destroy();
}

// 获取当前总扩散角度（委托给 SpreadHandler）
// 流程：调用 SpreadHandler.GetTotalSpreadAngle
float AWeaponActor::GetTotalSpreadAngle() const
{
    return SpreadHandler.GetTotalSpreadAngle(DataConfig);
}

// 设置武器操作锁定状态
// 流程：去重检查 → 设置 bWeaponActionsLocked → 锁定时清除按住标记并停火
void AWeaponActor::SetWeaponActionsLocked(bool bLocked)
{
    if (bWeaponActionsLocked == bLocked) return;
    bWeaponActionsLocked = bLocked;
    if (bLocked)
    {
        bFireInputHeld = false;
        StopFire();
    }
}

// 重置后坐力图案索引
// 流程：调用 RecoilHandler.Reset()
void AWeaponActor::ResetRecoilPattern()
{
    RecoilHandler.Reset();
}
