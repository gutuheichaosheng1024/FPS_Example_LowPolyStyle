#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "FPS_CharacterBase.generated.h"

class AWeaponActor;
enum class EWeaponTargetSlot : uint8;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponActivated, AWeaponActor*, Weapon);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCharacterDeath);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCharacterKilled, AFPS_CharacterBase*, Victim, AController*, KillerController);

UCLASS(config = Game)
class FPS_API AFPS_CharacterBase : public ACharacter
{
    GENERATED_BODY()

public:
    // ---------- 移动速度 ----------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "角色|状态")
    float WalkSpeed = 400.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "角色|状态")
    float SprintSpeed = 700.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "角色|状态")
    float AimSpeed = 200.f;

    // ---------- 状态标记 ----------
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

    // ---------- 血量 ----------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "角色|战斗",
              meta = (ClampMin = "1", ClampMax = "1000"))
    float MaxHealth = 100.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_CurrentHealth, Category = "角色|战斗")
    float CurrentHealth = 100.f;

    UFUNCTION(BlueprintPure, Category = "角色|战斗")
    bool IsDead() const { return bIsDead; }

    /** 角色死亡时广播 */
    UPROPERTY(BlueprintAssignable, Category = "角色|战斗")
    FOnCharacterDeath OnDeath;

    /** 角色被击杀时广播（携带击杀者控制器，供GameMode计分） */
    UPROPERTY(BlueprintAssignable, Category = "角色|战斗")
    FOnCharacterKilled OnKilled;

    /** 死亡后自动销毁延迟（秒），仅 AI 使用；0=不销毁 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "角色|战斗")
    float DeathDestroyDelay = 5.0f;

    // ---------- 武器 ----------
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

    // ---------- 脚步声 ----------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "角色|音频")
    USoundBase* FootstepSound;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "角色|音频", meta = (ClampMin = "0.1", ClampMax = "2.0"))
    float FootstepIntervalWalk = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "角色|音频", meta = (ClampMin = "0.1", ClampMax = "2.0"))
    float FootstepIntervalSprint = 0.35f;

    // ---------- 运动 (虚函数，子类可覆盖) ----------
    virtual void Sprint();
    virtual void StopSprint();
    virtual void Aim();
    virtual void StopAim();

    // ---------- 服务器 RPC (移动状态) ----------
    UFUNCTION(Server, Reliable, WithValidation)
    void Server_SetSprinting(bool bNewSprinting);

    UFUNCTION(Server, Reliable, WithValidation)
    void Server_SetAiming(bool bNewAiming);

    // ---------- 服务器 RPC (武器操作) ----------
    UFUNCTION(Server, Reliable, WithValidation)
    void Server_RequestSwitchWeapon(AWeaponActor* NewWeapon);

    UFUNCTION(Server, Reliable, WithValidation)
    void Server_RequestDrop(AWeaponActor* DroppedWeapon);

    // ---------- 死亡网络 RPC ----------
    UFUNCTION(NetMulticast, Reliable)
    void Multicast_OnDeath();

    // ---------- 拾取网络 RPC ----------
    UFUNCTION(Server, Reliable, WithValidation)
    void Server_RequestPickup(AWeaponActor* Weapon);

    // ---------- 网络复制回调 ----------
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

    // ---------- 多态接口 (玩家/ AI 返回不同数据源) ----------
    virtual FVector GetShootLocation() const;
    virtual FRotator GetShootRotation() const;
    virtual USkeletalMeshComponent* GetFPAnimMesh() const;

public:
    AFPS_CharacterBase();
    virtual void BeginPlay() override;

    /** 获取下一个唯一 TeamId（每次调用自增，确保每个角色 ID 不同） */
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

    /** 死亡处理（虚函数，子类覆盖 AI/玩家不同行为） */
    virtual void HandleDeath();

    FTimerHandle FootstepTimerHandle;
    FTimerHandle DeathDestroyTimerHandle;
    bool bFootstepActive = false;
    bool bIsDead = false;

    /** 最近一次伤害的施加者（TakeDamage中缓存，HandleDeath中通过OnKilled广播） */
    UPROPERTY(Transient)
    TObjectPtr<AController> LastDamageInstigator = nullptr;
};
