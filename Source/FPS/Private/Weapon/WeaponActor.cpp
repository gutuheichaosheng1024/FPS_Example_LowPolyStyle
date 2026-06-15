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
    // 客户端收到弹药更新后的回调
    // 可用于更新UI弹药显示
}

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

    // 初始弹夹可见性
    MeshManager.SetMagazineNormalState();

    if (!bIsEquipped)
    {
        SetActorTickEnabled(false);
        ScheduleAutoDestroy();
    }
}

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

// ---------- 装备 / 激活 / 丢弃 ----------
bool AWeaponActor::Equip(AFPS_CharacterBase* TargetCharacter)
{
    return HandleEquip(TargetCharacter);
}

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

void AWeaponActor::HandleActivateWeaponVisuals()
{
    SetWeaponActionsLocked(false);
    MeshManager.ShowWeaponMeshes(true, WeaponMesh, WeaponMeshStatic);
    // 弹夹状态：激活时根据换弹状态决定
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



// ---------- 输入绑定 ----------
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

void AWeaponActor::TryBindInput()
{
    if (!CachedInputComponent && bIsActiveWeapon && OwningCharacter)
        BindInput();
}

void AWeaponActor::OnFirePressed()
{
    bFireInputHeld = true;
    Fire();
}

void AWeaponActor::OnFireReleased()
{
    bFireInputHeld = false;
    StopFire();
}

// ---------- 玩家动作 ----------
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

void AWeaponActor::Reload()
{
    // ---------- 客户端预测（纯表现层）----------
    if (!OwningCharacter || !DataConfig) return;
    if (OwningCharacter->Reloading || OwningCharacter->Sprinting) return;
    if (CurrentAmmo >= DataConfig->MaxAmmo || TotalAmmo <= 0) return;

    // 本地立即设置状态（无延迟）
    ResetRecoilPattern();
    OwningCharacter->Reloading = true;
    OwningCharacter->Aiming = false;
    OwningCharacter->ApplyMovementSpeed();

    // 客户端弹夹可见性（换弹中：两个弹夹都可见）
    MeshManager.SetMagazineReloadState();

    // 本地播放 FP 动画 + 音效
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

    // 通知服务器
    if (!OwningCharacter->HasAuthority())
    {
        Server_RequestReload();
    }
    else
    {
        // 服务器本地（AI）→ 直接走服务器路径
        HandleReloadRequest();
    }
}

void AWeaponActor::View()
{
    if (bWeaponActionsLocked || !OwningCharacter || !OwningCharacter->GetController() || !bIsActiveWeapon || !AnimationConfig)
        return;

    bFireInputHeld = false;
    OwningCharacter->Viewing = true;
    OwningCharacter->Aiming = false;

    // 本地播放 FP 动画 + 音效
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

    // 通知服务器同步状态
    if (!OwningCharacter->HasAuthority())
        Server_RequestView(true);
}

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

    // 通知服务器同步状态
    if (!OwningCharacter->HasAuthority())
        Server_RequestView(false);
}

// ---------- 射击核心 ----------
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

    // 防止 StopFire/StartFire 快速交替导致的重复射击
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

void AWeaponActor::HandleStopFire()
{
    if (GetWorldTimerManager().IsTimerActive(AutoFireTimerHandle))
        GetWorldTimerManager().ClearTimer(AutoFireTimerHandle);
}

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

void AWeaponActor::PerformFire_Local()
{
    if (!OwningCharacter) return;

    LastFireTime = GetWorld()->GetTimeSeconds();

    // 计算射击方向（含扩散）
    const FVector Forward = OwningCharacter->GetShootRotation().Vector();
    FVector ShotDir = Forward;
    float Spread = SpreadHandler.GetTotalSpreadAngle(DataConfig);
    if (Spread > KINDA_SMALL_NUMBER)
    {
        ShotDir = UKismetMathLibrary::RandomUnitVectorInConeInRadians(Forward, FMath::DegreesToRadians(Spread));
    }

    // 服务器本地执行（AI / Listen Server 主机）→ 直接走权威路径
    if (OwningCharacter->HasAuthority())
    {
        // 本地 LineTrace 用于 VFX 命中点预测
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
            // Listen Server 主机：FP 动画 + 音效 + VFX + 后坐力 + 扩散
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
            // AI：TP 动画 + 音效 + VFX（无 FP 动画、无相机后坐力）
            if (AnimationConfig)
                PlayTPCharacterMontage(AnimationConfig->TP_FireAnimation);
            if (AudioConfig) PlaySound(AudioConfig->FireSound);
            HandleFireVFX(AuthorityImpactPoint);
        }

        // 服务器权威：射线检测 + 伤害
        Server_RequestFire_Implementation(OwningCharacter->GetShootLocation(), ShotDir);
        HandleFireFinished();
        return;
    }

    // ---------- 客户端预测（纯表现层）----------
    CurrentAmmo = FMath::Max(CurrentAmmo - 1, 0);

    // 本地 LineTrace（仅用于表现层 VFX 预测，不影响伤害）
    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(OwningCharacter);
    Params.AddIgnoredActor(this);
    const FVector Start = OwningCharacter->GetShootLocation();
    const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, Start + ShotDir * 10000.f, ECC_GameTraceChannel1, Params);
    const FVector ImpactPoint = bHit ? Hit.ImpactPoint : (Start + ShotDir * 5000.f);

    // 本地播放动画和音效（无延迟）
    const bool bWasAiming = OwningCharacter->Aiming;
    if (AnimationConfig)
    {
        PlayFPCharacterAndWeaponMontage(bWasAiming ? AnimationConfig->C_FireAimedAnimation : AnimationConfig->C_FireAnimation, AnimationConfig->W_FireAnimation);
    }
    if (AudioConfig) PlaySound(AudioConfig->FireSound);
    HandleFireVFX(ImpactPoint);
    SpreadHandler.AddSpreadOnFire(DataConfig);
    RecoilHandler.ApplyRecoil(DataConfig);

    // 发送给服务器（服务器做权威射线检测和伤害）
    Server_RequestFire(Start, ShotDir);

    HandleFireFinished();
}

bool AWeaponActor::Server_RequestFire_Validate(FVector ShootLocation, FVector ShotDirection)
{
    return OwningCharacter != nullptr && !OwningCharacter->IsDead();
}

void AWeaponActor::Server_RequestFire_Implementation(FVector ShootLocation, FVector ShotDirection)
{
    if (!OwningCharacter || !DataConfig) return;

    // 服务器权威扣弹药
    CurrentAmmo = FMath::Max(CurrentAmmo - 1, 0);

    // 服务器权威射线检测
    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(OwningCharacter);
    Params.AddIgnoredActor(this);
    const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, ShootLocation, ShootLocation + ShotDirection * 10000.f, ECC_GameTraceChannel1, Params);

    if (bHit)
        ApplyDamage(Hit, ShotDirection);

    // AI 听觉噪音
    UAISense_Hearing::ReportNoiseEvent(GetWorld(), OwningCharacter->GetActorLocation(),
        1.0f, OwningCharacter, 2500.0f, TEXT("Gunshot"));

    // 广播给远程客户端播放枪声/VFX
    Multicast_OnFire(bHit ? Hit.ImpactPoint : (ShootLocation + ShotDirection * 5000.f), bHit);
}

void AWeaponActor::Multicast_OnFire_Implementation(FVector ImpactPoint, bool bHit)
{
    // 跳过本地射击者（已本地预测播放）
    if (OwningCharacter && OwningCharacter->IsLocallyControlled()) return;

    // 远程客户端：播放 TP 蒙太奇 + 枪声 + VFX
    if (AnimationConfig)
        PlayTPCharacterMontage(AnimationConfig->TP_FireAnimation);
    if (AudioConfig) PlaySound(AudioConfig->FireSound);
    HandleFireVFX(ImpactPoint);
}

// ---------- 换弹网络 RPC ----------
bool AWeaponActor::Server_RequestReload_Validate()
{
    return OwningCharacter != nullptr && !OwningCharacter->IsDead();
}

void AWeaponActor::Server_RequestReload_Implementation()
{
    HandleReloadRequest();
}

void AWeaponActor::Multicast_OnReloadComplete_Implementation()
{
    // 所有客户端（含本地玩家）恢复弹夹可见性
    MeshManager.SetMagazineNormalState();
}

// ---------- 检视网络 RPC ----------
bool AWeaponActor::Server_RequestView_Validate(bool bNewViewing)
{
    return OwningCharacter != nullptr;
}

void AWeaponActor::Server_RequestView_Implementation(bool bNewViewing)
{
    // 服务器只转发状态，不播放动画
    OwningCharacter->Viewing = bNewViewing;
    if (bNewViewing)
        OwningCharacter->Aiming = false;
}

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

void AWeaponActor::ApplyDamage(const FHitResult& Hit, const FVector& ShotDirection)
{
    if (!DataConfig) return;

    // 对物理模拟组件施加冲量（环境物体、可破坏物等）
    if (UPrimitiveComponent* HitComp = Hit.GetComponent())
    {
        if (HitComp->IsSimulatingPhysics())
        {
            HitComp->AddImpulseAtLocation(ShotDirection.GetSafeNormal() * 20000.f, Hit.ImpactPoint);
        }
    }

    // 对 Actor 施加伤害
    if (AActor* HitActor = Hit.GetActor())
    {
        float FinalDamage = FMath::Max(DataConfig->BaseDamage + FMath::FRandRange(-DataConfig->DamageVariance, DataConfig->DamageVariance), 0.f);
        UGameplayStatics::ApplyPointDamage(HitActor, FinalDamage, ShotDirection.GetSafeNormal(), Hit,
            OwningCharacter ? OwningCharacter->GetController() : nullptr, this, UDamageType::StaticClass());
    }
}

void AWeaponActor::NotifyOwnerOutOfAmmo()
{
    if (OwningCharacter)
        OwningCharacter->HandleActiveWeaponOutOfAmmo(this); // 假设存在该方法，否则可移除
}

// ---------- 换弹 ----------
void AWeaponActor::HandleReloadRequest()
{
    // 服务器路径：设置状态 + TP 动画 + 启动计时器
    if (bWeaponActionsLocked || !OwningCharacter || !DataConfig) return;

    OwningCharacter->Reloading = true;
    OwningCharacter->Aiming = false;
    OwningCharacter->ApplyMovementSpeed();

    MeshManager.SetMagazineReloadState();

    // TP 动画（服务器本地 Mesh）
    const bool bEmptyReload = (CurrentAmmo == 0);
    if (AnimationConfig)
    {
        UAnimMontage* TP_Montage = (bEmptyReload && AnimationConfig->TP_ReloadEmptyAnimation) ? AnimationConfig->TP_ReloadEmptyAnimation : AnimationConfig->TP_ReloadAnimation;
        PlayTPCharacterMontage(TP_Montage);
    }

    // 用 FP 动画时长作为计时器（与客户端预测同步）
    UAnimMontage* C_Montage = nullptr;
    if (AnimationConfig)
        C_Montage = (bEmptyReload && AnimationConfig->C_ReloadEmptyAnimation) ? AnimationConfig->C_ReloadEmptyAnimation : AnimationConfig->C_ReloadAnimation;
    float Duration = C_Montage ? C_Montage->GetPlayLength() : 2.0f;
    GetWorldTimerManager().SetTimer(ReloadTimerHandle, this, &AWeaponActor::HandleReloadComplete, Duration, false);
}

void AWeaponActor::HandleReloadComplete()
{
    GetWorldTimerManager().ClearTimer(ReloadTimerHandle);
    if (!OwningCharacter) return;

    // 服务器权威：补充弹药 + 清除状态
    RefillAmmo();
    OwningCharacter->Reloading = false;
    OwningCharacter->ApplyMovementSpeed();

    // 恢复弹夹可见性（服务器本地）
    MeshManager.SetMagazineNormalState();

    // 广播给远程客户端恢复弹夹可见性
    Multicast_OnReloadComplete();

    OnWeaponReady.Broadcast();
}

void AWeaponActor::RefillAmmo()
{
    int Needed = DataConfig->MaxAmmo - CurrentAmmo;
    int Transfer = FMath::Min(Needed, TotalAmmo);
    TotalAmmo -= Transfer;
    CurrentAmmo += Transfer;
}

// ---------- 蒙太奇辅助 ----------
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

void AWeaponActor::PlayTPCharacterMontage(UAnimMontage* CharMontage)
{
    if (!OwningCharacter || !CharMontage) return;
    if (USkeletalMeshComponent* Mesh = OwningCharacter->GetMesh())
    {
        if (UAnimInstance* Anim = Mesh->GetAnimInstance())
            Anim->Montage_Play(CharMontage);
    }
}

void AWeaponActor::OnFireMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    HandleFireFinished();
}

void AWeaponActor::OnReloadMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    // 可能被中断，不做处理；真正的完成在定时器
}

void AWeaponActor::OnViewMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    HandleViewFinished();
}

void AWeaponActor::HandleFireFinished()
{
    if (OwningCharacter)
        OwningCharacter->CanSprint = true;
    OnWeaponReady.Broadcast(); // 简化：直接广播
}

void AWeaponActor::HandleReloadFinished()
{
    if (OwningCharacter)
        OwningCharacter->Reloading = false;
    MeshManager.SetMagazineNormalState();
}

void AWeaponActor::HandleViewFinished()
{
    if (OwningCharacter)
        OwningCharacter->Viewing = false;
}

// ---------- 音效 ----------
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

void AWeaponActor::StopActiveSound()
{
    if (ActiveAudioComponent.IsValid())
    {
        ActiveAudioComponent->Stop();
    }
    ActiveAudioComponent = nullptr;
}

// ---------- 视觉效果 ----------
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

// ---------- 拾取与掉落物理 ----------
void AWeaponActor::HandlePickupComponentPostEquip()
{
    if (WeaponMeshStatic)
    {
        WeaponMeshStatic->SetSimulatePhysics(false);
        WeaponMeshStatic->SetCollisionProfileName(TEXT("NoCollision"));
    }
    EnablePickupComponent(false);
}

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

// ---------- 自动销毁 ----------
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

void AWeaponActor::CancelAutoDestroy()
{
    GetWorldTimerManager().ClearTimer(AutoDestroyTimerHandle);
}

void AWeaponActor::HandleAutoDestroyTimerExpired()
{
    if (!bIsEquipped)
        Destroy();
}

// ---------- 扩散（委托给 SpreadHandler） ----------
float AWeaponActor::GetTotalSpreadAngle() const
{
    return SpreadHandler.GetTotalSpreadAngle(DataConfig);
}

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

void AWeaponActor::ResetRecoilPattern()
{
    RecoilHandler.Reset();
}