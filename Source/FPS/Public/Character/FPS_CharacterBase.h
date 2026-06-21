#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "FPS_CharacterBase.generated.h"

class AWeaponActor;
enum class EWeaponTargetSlot : uint8;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponActivated, AWeaponActor*, Weapon);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCharacterDeath);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCharacterKilled, AFPS_CharacterBase*, Victim, AController*, KillerController);

/**
 * AFPS_CharacterBase — shared character base class for both player and AI
 *
 * 职责：血量管理与伤害处理、武器库存与切换、移动速度状态机（行走/冲刺/瞄准）、脚步声音效、死亡处理与网络多播、网络复制属性与RPC
 * 使用：AWeaponActor, UCharacterMovementComponent
 */
UCLASS(config = Game)
class FPS_API AFPS_CharacterBase : public ACharacter
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "角色|状态")
    float WalkSpeed = 400.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "角色|状态")
    float SprintSpeed = 700.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "角色|状态")
    float AimSpeed = 200.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_Sprinting, Category = "角色|状态")
    bool Sprinting = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "角色|状态")
    bool CanSprint = true;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_Aiming, Category = "角色|状态")
    bool Aiming = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "角色|状态")
    bool CanAim = true;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_Reloading, Category = "角色|状态")
    bool Reloading = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_Viewing, Category = "角色|状态")
    bool Viewing = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "角色|状态")
    bool bIsGrounded = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "角色|战斗",
              meta = (ClampMin = "1", ClampMax = "1000"))
    float MaxHealth = 100.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_CurrentHealth, Category = "角色|战斗")
    float CurrentHealth = 100.f;

    UFUNCTION(BlueprintPure, Category = "角色|战斗")
    bool IsDead() const { return bIsDead; }

    UPROPERTY(BlueprintAssignable, Category = "角色|战斗")
    FOnCharacterDeath OnDeath;

    UPROPERTY(BlueprintAssignable, Category = "角色|战斗")
    FOnCharacterKilled OnKilled;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "角色|战斗")
    float DeathDestroyDelay = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "角色|武器")
    TSubclassOf<AWeaponActor> DefaultWeaponClass;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_CurrentWeapon, Category = "角色|武器")
    AWeaponActor* CurrentWeapon = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "角色|武器", meta = (ClampMin = "1", ClampMax = "5"))
    int32 MaxWeaponCount = 2;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "角色|武器")
    TArray<AWeaponActor*> WeaponInventory;

    UPROPERTY(BlueprintAssignable, Category = "角色|武器")
    FOnWeaponActivated OnWeaponActivated;

    UFUNCTION(BlueprintCallable, Category = "角色|武器")
    bool EquipWeaponActor(AWeaponActor* Weapon);

    UFUNCTION(BlueprintPure, Category = "角色|武器")
    bool CanEquipWeaponInSlot(EWeaponTargetSlot Slot, const AWeaponActor* Weapon) const;

    void HandleActiveWeaponOutOfAmmo(AWeaponActor* DepletedWeapon);
    void SwitchWeapon();

    UFUNCTION(BlueprintCallable, Category = "角色|武器")
    bool SwitchToWeapon(AWeaponActor* Weapon);

    void DropCurrentWeapon();

    UFUNCTION(BlueprintPure, Category = "角色|武器")
    AWeaponActor* GetInactiveWeapon() const;

    UFUNCTION(BlueprintPure, Category = "角色|武器")
    bool HasWeaponOfClass(TSubclassOf<AWeaponActor> WeaponClass) const;

    UFUNCTION(BlueprintPure, Category = "角色|武器")
    int32 GetWeaponCount() const { return WeaponInventory.Num(); }

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "角色|音频")
    USoundBase* FootstepSound;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "角色|音频", meta = (ClampMin = "0.1", ClampMax = "2.0"))
    float FootstepIntervalWalk = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "角色|音频", meta = (ClampMin = "0.1", ClampMax = "2.0"))
    float FootstepIntervalSprint = 0.35f;

    virtual void Sprint();
    virtual void StopSprint();
    virtual void Aim();
    virtual void StopAim();

    UFUNCTION(Server, Reliable, WithValidation)
    void Server_SetSprinting(bool bNewSprinting);

    UFUNCTION(Server, Reliable, WithValidation)
    void Server_SetAiming(bool bNewAiming);

    UFUNCTION(Server, Reliable, WithValidation)
    void Server_RequestSwitchWeapon(AWeaponActor* NewWeapon);

    UFUNCTION(Server, Unreliable)
    void Server_SetRotation(FRotator NewRotation);

    UFUNCTION(Server, Reliable, WithValidation)
    void Server_RequestDrop(AWeaponActor* DroppedWeapon);

    UFUNCTION(NetMulticast, Reliable)
    void Multicast_OnDeath();

    UFUNCTION(Server, Reliable, WithValidation)
    void Server_RequestPickup(AWeaponActor* Weapon);

    UFUNCTION()
    void OnRep_CurrentHealth();

    UFUNCTION()
    void OnRep_CurrentWeapon();

    UFUNCTION()
    void OnRep_Sprinting();

    UFUNCTION()
    void OnRep_Aiming();

    UFUNCTION()
    void OnRep_Reloading();

    UFUNCTION()
    void OnRep_Viewing();

    virtual FVector GetShootLocation() const;
    virtual FRotator GetShootRotation() const;
    virtual USkeletalMeshComponent* GetFPAnimMesh() const;

public:
    AFPS_CharacterBase();
    virtual void BeginPlay() override;

    static uint8 GetNextTeamId();
    virtual void Tick(float DeltaTime) override;
    virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
        AController* EventInstigator, AActor* DamageCauser) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
    void ApplyMovementSpeed();

protected:
    void UpdateFootsteps();
    void PlayFootstep();

    virtual void HandleDeath();

    FTimerHandle FootstepTimerHandle;
    FTimerHandle DeathDestroyTimerHandle;
    bool bFootstepActive = false;
    bool bIsDead = false;

    UPROPERTY(Transient)
    TObjectPtr<AController> LastDamageInstigator = nullptr;
};
