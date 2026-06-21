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

/**
 * AFP_Character — player-controlled character with first-person camera and Enhanced Input
 *
 * 职责：第一人称摄像机与手臂网格、Enhanced Input 输入绑定与处理、冲刺/瞄准互斥状态机、IGenericTeamAgentInterface 队伍标识、死亡时布娃娃与输入禁用
 * 使用：UCameraComponent, UEnhancedInputComponent, UInputAction, UInputMappingContext, AFPS_CharacterBase
 */
UCLASS(config = Game)
class FPS_API AFP_Character : public AFPS_CharacterBase, public IGenericTeamAgentInterface
{
    GENERATED_BODY()

public:
    USkeletalMeshComponent* GetMesh1P() const { return Mesh1P; }
    UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

    virtual FVector GetShootLocation() const override;
    virtual FRotator GetShootRotation() const override;
    virtual USkeletalMeshComponent* GetFPAnimMesh() const override;

    virtual void Sprint() override;
    virtual void StopSprint() override;
    virtual void Aim() override;
    virtual void StopAim() override;

    void SetAimingStateInternal(bool bNewAiming);

    virtual void HandleDeath() override;
    virtual void Multicast_OnDeath_Implementation() override;

    virtual FGenericTeamId GetGenericTeamId() const override { return TeamId; }
    virtual void SetGenericTeamId(const FGenericTeamId& NewTeamId) override { TeamId = NewTeamId; }

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Mesh)
    USkeletalMeshComponent* Mesh1P;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    UCameraComponent* FirstPersonCameraComponent;

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

    bool isSprintHeld = false;
    bool isAimHeld = false;

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
