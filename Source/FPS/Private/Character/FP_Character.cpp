#include "Character/FP_Character.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GenericTeamAgentInterface.h"
#include "InputActionValue.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

// 构造函数：创建第一人称摄像机与手臂网格，配置可见性与TeamId
// 流程：初始化胶囊体 → 配置第三人称旋转跟随控制器 → 创建 FP 摄像机 → 创建 FP 手臂网格(仅拥有者可见) → 隐藏 TP 网格 → 分配唯一 TeamId
AFP_Character::AFP_Character()
{
    GetCapsuleComponent()->InitCapsuleSize(55.f, 96.f);

    bUseControllerRotationYaw = true;

    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->bOrientRotationToMovement = false;
    }

    FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
    FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
    FirstPersonCameraComponent->SetRelativeLocation(FVector(-10.f, 0.f, 60.f));
    FirstPersonCameraComponent->bUsePawnControlRotation = true;

    Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
    Mesh1P->SetOnlyOwnerSee(true);
    Mesh1P->SetupAttachment(FirstPersonCameraComponent);
    Mesh1P->bCastDynamicShadow = false;
    Mesh1P->CastShadow = false;
    Mesh1P->SetRelativeLocation(FVector(-30.f, 0.f, -150.f));

    if (USkeletalMeshComponent* ThirdPersonMesh = GetMesh())
    {
        ThirdPersonMesh->SetOwnerNoSee(true);
    }

    TeamId = FGenericTeamId(AFPS_CharacterBase::GetNextTeamId());
}

// 初始化：同步控制器 TeamId
// 流程：调用父类 BeginPlay → 将控制器的 TeamId 同步为角色的 TeamId
void AFP_Character::BeginPlay()
{
    Super::BeginPlay();

    if (AController* MyController = GetController())
    {
        if (IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(MyController))
        {
            TeamAgent->SetGenericTeamId(TeamId);
        }
    }
}

// 绑定 Enhanced Input 动作到处理函数
// 流程：获取 EnhancedInputComponent → 绑定 Jump/Move/Look/Sprint/Aim/SwitchWeapon/DropWeapon → 添加 DefaultMappingContext 到子系统
void AFP_Character::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

        EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AFP_Character::Move);
        EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AFP_Character::Look);

        if (SprintAction)
        {
            EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &AFP_Character::Sprint);
            EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &AFP_Character::StopSprint);
        }

        if (AimAction)
        {
            EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Started, this, &AFP_Character::Aim);
            EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &AFP_Character::StopAim);
        }

        if (SwitchWeaponAction)
        {
            EnhancedInputComponent->BindAction(SwitchWeaponAction, ETriggerEvent::Started, this, &AFP_Character::SwitchWeapon);
        }

        if (DropWeaponAction)
        {
            EnhancedInputComponent->BindAction(DropWeaponAction, ETriggerEvent::Started, this, &AFP_Character::DropCurrentWeapon);
        }
    }
    else
    {
        UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' requires Enhanced Input component."), *GetNameSafe(this));
    }

    if (APlayerController* PC = Cast<APlayerController>(Controller))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
            ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
        {
            if (DefaultMappingContext)
            {
                Subsystem->AddMappingContext(DefaultMappingContext, 0);
            }
        }
    }
}

// 处理移动输入
// 流程：获取二维输入向量 → 基于控制器 Yaw 构建前/右方向 → 调用 AddMovementInput
void AFP_Character::Move(const FInputActionValue& Value)
{
    const FVector2D MovementVector = Value.Get<FVector2D>();
    if (Controller != nullptr)
    {
        const FRotator ControlRotation = Controller->GetControlRotation();
        const FRotator YawRotation(0.0, ControlRotation.Yaw, 0.0);
        const FVector ForwardDir = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
        const FVector RightDir = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

        AddMovementInput(ForwardDir, MovementVector.Y);
        AddMovementInput(RightDir, MovementVector.X);
    }
}

// 处理视角旋转输入
// 流程：获取二维输入向量 → 应用 Yaw/Pitch → 通过 Server RPC 同步控制器旋转到服务器
void AFP_Character::Look(const FInputActionValue& Value)
{
    const FVector2D LookAxisVector = Value.Get<FVector2D>();
    if (Controller != nullptr)
    {
        AddControllerYawInput(LookAxisVector.X);
        AddControllerPitchInput(-LookAxisVector.Y);

        Server_SetRotation(Controller->GetControlRotation());
    }
}

// 玩家冲刺（输入驱动的互斥状态机）
// 流程：检查换弹中则忽略 → 本地设置冲刺状态 → 通知服务器
void AFP_Character::Sprint()
{
    if (Reloading) return;
    SetSprintingStateInternal(true);
    Server_SetSprinting(true);
}

// 玩家停止冲刺
// 流程：本地清除冲刺状态 → 通知服务器
void AFP_Character::StopSprint()
{
    SetSprintingStateInternal(false);
    Server_SetSprinting(false);
}

// 冲刺状态内部切换（冲刺与瞄准互斥）
// 流程：更新按键保持标记 → 状态未变化则忽略 → 更新 Sprinting → 冲刺时取消瞄准 → 停止冲刺时恢复瞄准按键状态 → 应用移动速度
void AFP_Character::SetSprintingStateInternal(bool bNewSprinting)
{
    isSprintHeld = bNewSprinting;
    if (Sprinting == bNewSprinting) return;

    Sprinting = bNewSprinting;
    if (bNewSprinting)
    {
        Aiming = false;
    }
    else
    {
        CanAim = true;
        Aiming = isAimHeld;
    }
    ApplyMovementSpeed();
}

// 玩家瞄准
// 流程：本地设置瞄准状态 → 通知服务器
void AFP_Character::Aim()
{
    SetAimingStateInternal(true);
    Server_SetAiming(true);
}

// 玩家停止瞄准
// 流程：本地清除瞄准状态 → 通知服务器
void AFP_Character::StopAim()
{
    SetAimingStateInternal(false);
    Server_SetAiming(false);
}

// 瞄准状态内部切换（瞄准与冲刺互斥）
// 流程：更新按键保持标记 → 状态未变化则忽略 → 更新 Aiming → 瞄准时取消冲刺 → 停止瞄准时恢复冲刺按键状态 → 应用移动速度
void AFP_Character::SetAimingStateInternal(bool bNewAiming)
{
    isAimHeld = bNewAiming;
    if (Aiming == bNewAiming) return;

    Aiming = bNewAiming;
    if (bNewAiming)
    {
        Sprinting = false;
    }
    else
    {
        CanSprint = true;
        Sprinting = isSprintHeld;
    }
    ApplyMovementSpeed();
}

// 获取射击起点（玩家：第一人称摄像机位置）
// 流程：返回 FirstPersonCameraComponent 的世界位置，备用返回 ActorLocation
FVector AFP_Character::GetShootLocation() const
{
    return FirstPersonCameraComponent ? FirstPersonCameraComponent->GetComponentLocation() : GetActorLocation();
}

// 获取射击旋转（玩家：控制器旋转）
// 流程：返回 GetControlRotation
FRotator AFP_Character::GetShootRotation() const
{
    return GetControlRotation();
}

// 获取第一人称动画网格（玩家：手臂网格 Mesh1P）
// 流程：返回 Mesh1P
USkeletalMeshComponent* AFP_Character::GetFPAnimMesh() const
{
    return Mesh1P;
}

// 玩家死亡处理
// 流程：调用基类 HandleDeath（禁用碰撞/移动 + 广播 OnKilled）
void AFP_Character::HandleDeath()
{
    Super::HandleDeath();
}

// 玩家死亡多播表现
// 流程：调用基类 Multicast_OnDeath → 禁用输入 → 销毁 FP 手臂网格 → TP 网格启用布娃娃物理
void AFP_Character::Multicast_OnDeath_Implementation()
{
    Super::Multicast_OnDeath_Implementation();

    if (APlayerController* PC = Cast<APlayerController>(Controller))
    {
        DisableInput(PC);
    }

    if (Mesh1P)
    {
        Mesh1P->DestroyComponent();
    }

    if (USkeletalMeshComponent* MeshTP = GetMesh())
    {
        MeshTP->SetOwnerNoSee(false);
        MeshTP->SetSimulatePhysics(true);
        MeshTP->SetCollisionProfileName(TEXT("Ragdoll"));
        MeshTP->SetAnimInstanceClass(nullptr);
    }
}
