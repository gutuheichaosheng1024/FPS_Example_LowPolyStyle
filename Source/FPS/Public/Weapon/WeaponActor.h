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

UCLASS(BlueprintType)
class FPS_API AWeaponActor : public AActor
{
    GENERATED_BODY()

public:
    AWeaponActor();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

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
    void NotifyOwnerOutOfAmmo();

    /** 重置后坐力图案索引（换弹/切换武器时调用） */
    UFUNCTION(BlueprintCallable, Category = "Weapon|Recoil")
    void ResetRecoilPattern();

    UPROPERTY(BlueprintAssignable)
    FOnWeaponReady OnWeaponReady;

    void TryBindInput();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaTime) override;

    // 功能函数
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

    // 装备/丢弃流程
    bool HandleEquip(AFPS_CharacterBase* TargetCharacter);
    void HandleActivateWeapon();
    void HandleDeactivateWeapon();
    void HandleDropWeapon();

    // 射击核心
    void HandleStartFire();
    void HandleStopFire();
    void HandleAutoFire();
    void PerformFire_Local();
    bool PerformLineTrace(FHitResult& OutHit, FVector& OutShotDirection) const;
    void ApplyDamage(const FHitResult& Hit, const FVector& ShotDirection);

    // 射击网络 RPC
    UFUNCTION(Server, Reliable, WithValidation)
    void Server_RequestFire(FVector ShootLocation, FVector ShotDirection);

    UFUNCTION(NetMulticast, Unreliable)
    void Multicast_OnFire(FVector ImpactPoint, bool bHit);

    // 换弹核心
    void HandleReloadRequest();
    void HandleReloadComplete();

    // 换弹网络 RPC
    UFUNCTION(Server, Reliable, WithValidation)
    void Server_RequestReload();

    UFUNCTION(NetMulticast, Reliable)
    void Multicast_OnReloadComplete();

    // 检视网络 RPC
    UFUNCTION(Server, Reliable, WithValidation)
    void Server_RequestView(bool bNewViewing);

    // 网络复制回调
    UFUNCTION()
    void OnRep_CurrentAmmo();

    // 拾取与物理
    void HandlePickupComponentPostEquip();
    void EnablePickupComponent(bool bEnable);
    void ApplyDroppedPhysicsState();
    void DropWeaponPhysically(const FVector& ViewLoc, const FRotator& ViewRot);

    // 自动销毁
    void ScheduleAutoDestroy();
    void CancelAutoDestroy();
    void HandleAutoDestroyTimerExpired();

    // 扩散（委托给 SpreadHandler）
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

    /** 武器数据配置（伤害/弹药/扩散/后坐力） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
    TObjectPtr<UWeaponDataConfig> DataConfig;

private:

    // 武器属性
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

    // 输入
    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction* FireAction;
    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction* ReloadAction;
    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction* ViewAction;

    // 音频配置 DataAsset
    UPROPERTY(EditAnywhere, Category = "Audio")
    TObjectPtr<UWeaponAudioConfig> AudioConfig;

    // 动画配置 DataAsset
    UPROPERTY(EditAnywhere, Category = "Animation")
    TObjectPtr<UWeaponAnimationConfig> AnimationConfig;

    UPROPERTY(EditAnywhere)
    FName AttachSocketName = TEXT("SOCKET_Weapon");

    // 状态
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

    // 定时器
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

    // ---------- 后坐力 ----------
    FWeaponRecoilHandler RecoilHandler;

    // ---------- 扩散 ----------
    FWeaponSpreadHandler SpreadHandler;

    // ---------- 网格管理 ----------
    FWeaponMeshManager MeshManager;
};