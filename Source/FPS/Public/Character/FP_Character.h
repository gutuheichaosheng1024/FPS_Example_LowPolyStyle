#pragma once

#include "CoreMinimal.h"
#include "Character/FPS_CharacterBase.h"
#include "GenericTeamAgentInterface.h"
#include "FP_Character.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(config = Game)
class FPS_API AFP_Character : public AFPS_CharacterBase, public IGenericTeamAgentInterface
{
    GENERATED_BODY()

public:
    // ---------- 获取组件 ----------
    USkeletalMeshComponent* GetMesh1P() const { return Mesh1P; }
    UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

    // ---------- 多态接口覆盖 ----------
    virtual FVector GetShootLocation() const override;
    virtual FRotator GetShootRotation() const override;
    virtual USkeletalMeshComponent* GetFPAnimMesh() const override;

    // ---------- 运动覆盖 (输入驱动的互斥状态机) ----------
    virtual void Sprint() override;
    virtual void StopSprint() override;
    virtual void Aim() override;
    virtual void StopAim() override;

    void SetAimingStateInternal(bool bNewAiming);

    virtual void HandleDeath() override;
    virtual void Multicast_OnDeath_Implementation() override;

    // ---------- IGenericTeamAgentInterface ----------
    virtual FGenericTeamId GetGenericTeamId() const override { return TeamId; }
    virtual void SetGenericTeamId(const FGenericTeamId& NewTeamId) override { TeamId = NewTeamId; }

protected:
    // 第一人称手臂网格
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Mesh)
    USkeletalMeshComponent* Mesh1P;

    // 第一人称摄像机
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    UCameraComponent* FirstPersonCameraComponent;

    // 输入映射
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
    UInputMappingContext* DefaultMappingContext;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
    UInputAction* JumpAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
    UInputAction* MoveAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
    UInputAction* LookAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
    UInputAction* SprintAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
    UInputAction* AimAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
    UInputAction* SwitchWeaponAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
    UInputAction* DropWeaponAction;

    // 按键保持状态
    bool isSprintHeld = false;
    bool isAimHeld = false;

    /** 玩家队伍 ID（AI 用此判断敌我，默认 0=与 AI 的 TeamId=1 互斥） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
    FGenericTeamId TeamId = FGenericTeamId(0);

public:
    AFP_Character();
    virtual void BeginPlay() override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    void Move(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);

protected:
    void SetSprintingStateInternal(bool bNewSprinting);
};
