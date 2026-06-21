#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapon/WeaponRecoilHandler.h"
#include "Weapon/WeaponSpreadHandler.h"
#include "Weapon/WeaponMeshManager.h"
#include "WeaponActor.generated.h"

class AFPS_CharacterBase;
class UAnimInstance;
class USkeletalMeshComponent;
class UAudioComponent;
class UAnimMontage;
class UInputAction;
class UParticleSystem;
class UPickUpComponent;
class UWeaponAnimationConfig;
class UWeaponAudioConfig;
class UWeaponDataConfig;
struct FEnhancedInputActionEventBinding;
class UEnhancedInputComponent;

UENUM(BlueprintType)
enum class EWeaponTargetSlot : uint8
{
    Primary     UMETA(DisplayName = "Primary"),
    Secondary   UMETA(DisplayName = "Secondary")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWeaponReady);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHitConfirmed, bool, bKilled);

/**
 * AWeaponActor — 武器 Actor，管理武器的装备、射击、换弹、检视、丢弃全生命周期
 *
 * 职责：处理武器状态机（装备/激活/停用/丢弃）、射击系统（含扩散和后坐力）、弹药管理、动画/音效/VFX 播放、
 *       网络复制（Server RPC 权威射击/换弹/检视，Multicast 广播表现）、EnhancedInput 绑定、拾取交互和掉落物理
 * 使用：AFPS_CharacterBase（持有者角色）、UWeaponDataConfig（数据配置）、UWeaponAnimationConfig（动画配置）、
 *       UWeaponAudioConfig（音频配置）、FWeaponRecoilHandler（后坐力）、FWeaponSpreadHandler（扩散）、
 *       FWeaponMeshManager（网格管理）、UPickUpComponent（拾取组件）、UEnhancedInputComponent（输入绑定）
 */
UCLASS(BlueprintType)
class FPS_API AWeaponActor : public AActor
{
    GENERATED_BODY()

public:
    AWeaponActor();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void OnRep_Owner() override;

    bool Equip(AFPS_CharacterBase* TargetCharacter);
    void ActivateWeapon();
    void DeactivateWeapon();
    void DropWeapon();

    bool IsActiveWeapon() const { return bIsActiveWeapon; }
    EWeaponTargetSlot GetTargetSlot() const { return TargetSlot; }

    void Fire();
    void StopFire();
    void Reload();
    void View();
    void StopView();

    void SetWeaponActionsLocked(bool bLocked);
    bool AreWeaponActionsLocked() const { return bWeaponActionsLocked; }

    bool HasAnyAmmo() const { return CurrentAmmo > 0 || TotalAmmo > 0; }

    UFUNCTION(BlueprintPure, Category = "Weapon|Audio")
    UWeaponAudioConfig* GetAudioConfig() const { return AudioConfig; }
    void NotifyOwnerOutOfAmmo();

    UFUNCTION(BlueprintCallable, Category = "Weapon|Recoil")
    void ResetRecoilPattern();

    UPROPERTY(BlueprintAssignable)
    FOnWeaponReady OnWeaponReady;

    UPROPERTY(BlueprintAssignable, Category = "Weapon|Combat")
    FOnHitConfirmed OnHitConfirmed;

    void TryBindInput();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaTime) override;

    void Unholster();
    void BindInput();
    void UnbindInput();

    float PlayFPCharacterAndWeaponMontage(UAnimMontage* CharMontage, UAnimMontage* WeaponMontage);
    void PlayTPCharacterMontage(UAnimMontage* CharMontage);
    void PlaySound(USoundBase* SoundToPlay);
    void StopActiveSound();

    void HandleFireFinished();
    void HandleReloadFinished();
    void RefillAmmo();
    void HandleViewFinished();

    void HandleActivateWeaponVisuals();
    void HandleDeactivateWeaponVisuals();

    void HandleFireVFX(const FVector& HitPointLocal);

    void OnFirePressed();
    void OnFireReleased();

    void OnFireMontageEnded(UAnimMontage* Montage, bool bInterrupted);
    void OnReloadMontageEnded(UAnimMontage* Montage, bool bInterrupted);
    void OnViewMontageEnded(UAnimMontage* Montage, bool bInterrupted);

    bool HandleEquip(AFPS_CharacterBase* TargetCharacter);
    void HandleActivateWeapon();
    void HandleDeactivateWeapon();
    void HandleDropWeapon();

    void HandleStartFire();
    void HandleStopFire();
    void HandleAutoFire();
    void PerformFire_Local();
    bool PerformLineTrace(FHitResult& OutHit, FVector& OutShotDirection) const;
    void ApplyDamage(const FHitResult& Hit, const FVector& ShotDirection);

    UFUNCTION(Server, Reliable, WithValidation)
    void Server_RequestFire(FVector ShootLocation, FVector ShotDirection);

    UFUNCTION(NetMulticast, Unreliable)
    void Multicast_OnFire(FVector ImpactPoint, bool bHit);

    void HandleReloadRequest();
    void HandleReloadComplete();

    UFUNCTION(Server, Reliable, WithValidation)
    void Server_RequestReload();

    UFUNCTION(NetMulticast, Reliable)
    void Multicast_OnReloadComplete();

    UFUNCTION(Server, Reliable, WithValidation)
    void Server_RequestView(bool bNewViewing);

    UFUNCTION()
    void OnRep_CurrentAmmo();

    void HandlePickupComponentPostEquip();
    void EnablePickupComponent(bool bEnable);
    void ApplyDroppedPhysicsState();
    void DropWeaponPhysically(const FVector& ViewLoc, const FRotator& ViewRot);

    void ScheduleAutoDestroy();
    void CancelAutoDestroy();
    void HandleAutoDestroyTimerExpired();

    UFUNCTION(BlueprintCallable)
    float GetTotalSpreadAngle() const;

public:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    USceneComponent* Root;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    USkeletalMeshComponent* WeaponMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    USkeletalMeshComponent* WeaponMeshStatic;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    UPickUpComponent* PickupComponent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
    TObjectPtr<UWeaponDataConfig> DataConfig;

private:

    UPROPERTY(EditDefaultsOnly)
    EWeaponTargetSlot TargetSlot = EWeaponTargetSlot::Primary;

    UPROPERTY(EditDefaultsOnly)
    UParticleSystem* FireVFX;

public:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Weapon|Ammo")
    int32 TotalAmmo = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_CurrentAmmo, Category = "Weapon|Ammo")
    int32 CurrentAmmo = 0;

private:

    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction* FireAction;
    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction* ReloadAction;
    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction* ViewAction;

    UPROPERTY(EditAnywhere, Category = "Audio")
    TObjectPtr<UWeaponAudioConfig> AudioConfig;

    UPROPERTY(EditAnywhere, Category = "Animation")
    TObjectPtr<UWeaponAnimationConfig> AnimationConfig;

    UPROPERTY(EditAnywhere)
    FName AttachSocketName = TEXT("SOCKET_Weapon");

    bool bIsEquipped = false;
    bool bIsActiveWeapon = false;
    AFPS_CharacterBase* OwningCharacter = nullptr;

    UPROPERTY()
    UEnhancedInputComponent* CachedInputComponent = nullptr;

    FEnhancedInputActionEventBinding* FirePressedBinding = nullptr;
    FEnhancedInputActionEventBinding* FireReleasedBinding = nullptr;
    FEnhancedInputActionEventBinding* ReloadBinding = nullptr;
    FEnhancedInputActionEventBinding* ViewBinding = nullptr;

    TWeakObjectPtr<UAudioComponent> ActiveAudioComponent;
    bool bSkipNextSound = false;
    bool bFireInputHeld = false;

    FTimerHandle AutoFireTimerHandle;
    FTimerHandle ReloadTimerHandle;
    FTimerHandle PickupReactivateHandle;
    FTimerHandle AutoDestroyTimerHandle;

    bool bPickupComponentDisabled = false;
    bool bWeaponActionsLocked = false;
    float LastFireTime = -100.f;

    FOnMontageEnded ReloadMontageEndedDelegate;
    FOnMontageEnded ViewMontageEndedDelegate;
    FOnMontageEnded FireMontageEndedDelegate;

    UPROPERTY(EditDefaultsOnly)
    float UnpickedLifeSpan = 5.f;

    FWeaponRecoilHandler RecoilHandler;
    FWeaponSpreadHandler SpreadHandler;
    FWeaponMeshManager MeshManager;
};
