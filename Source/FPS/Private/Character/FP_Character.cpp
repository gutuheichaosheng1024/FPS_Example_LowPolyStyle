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

AFP_Character::AFP_Character()
{
    GetCapsuleComponent()->InitCapsuleSize(55.f, 96.f);

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
}

void AFP_Character::BeginPlay()
{
    Super::BeginPlay();

    // 确保控制器与角色 TeamId 一致（控制器在 Possess 后可能尚未同步）
    if (AController* MyController = GetController())
    {
        if (IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(MyController))
        {
            TeamAgent->SetGenericTeamId(TeamId);
        }
    }
}

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

void AFP_Character::Look(const FInputActionValue& Value)
{
    const FVector2D LookAxisVector = Value.Get<FVector2D>();
    if (Controller != nullptr)
    {
        AddControllerYawInput(LookAxisVector.X);
        AddControllerPitchInput(-LookAxisVector.Y);
    }
}

// ---------- 运动覆盖 (输入驱动的互斥状态机) ----------
void AFP_Character::Sprint()
{
    if (Reloading) return;
    SetSprintingStateInternal(true);       // 本地预测
    Server_SetSprinting(true);             // 通知服务器
}

void AFP_Character::StopSprint()
{
    SetSprintingStateInternal(false);      // 本地预测
    Server_SetSprinting(false);            // 通知服务器
}

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

void AFP_Character::Aim()
{
    SetAimingStateInternal(true);          // 本地预测
    Server_SetAiming(true);                // 通知服务器
}

void AFP_Character::StopAim()
{
    SetAimingStateInternal(false);         // 本地预测
    Server_SetAiming(false);               // 通知服务器
}

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

// ---------- 多态接口 ----------
FVector AFP_Character::GetShootLocation() const
{
    return FirstPersonCameraComponent ? FirstPersonCameraComponent->GetComponentLocation() : GetActorLocation();
}

FRotator AFP_Character::GetShootRotation() const
{
    return GetControlRotation();
}

USkeletalMeshComponent* AFP_Character::GetFPAnimMesh() const
{
    return Mesh1P;
}

void AFP_Character::HandleDeath()
{
	Super::HandleDeath();  // 禁用碰撞+移动 + 广播OnKilled（触发GM计分+重生UI推送）
}

void AFP_Character::Multicast_OnDeath_Implementation()
{
	Super::Multicast_OnDeath_Implementation();

	// 禁用玩家输入（客户端本地）
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		DisableInput(PC);
	}

	// 销毁 FP 网格（FP武器随Mesh1P销毁自动清理）
	if (Mesh1P)
	{
		Mesh1P->DestroyComponent();
	}

	// TP 网格：布娃娃（角色由GameMode延迟销毁，布娃娃随之清理）
	if (USkeletalMeshComponent* MeshTP = GetMesh())
	{
		MeshTP->SetOwnerNoSee(false);
		MeshTP->SetSimulatePhysics(true);
		MeshTP->SetCollisionProfileName(TEXT("Ragdoll"));
		MeshTP->SetAnimInstanceClass(nullptr);
	}
}
