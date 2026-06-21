#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WeaponAnimationConfig.generated.h"

class UAnimInstance;
class UAnimMontage;

/**
 * UWeaponAnimationConfig — 武器动画配置 DataAsset
 *
 * 职责：配置武器相关的所有动画资源，包括角色 FP/TP AnimBlueprint 类以及出枪、射击、换弹、检视等蒙太奇
 * 使用：UAnimInstance（动画蓝图基类）、UAnimMontage（动画蒙太奇）
 */
UCLASS(BlueprintType)
class FPS_API UWeaponAnimationConfig : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    TSubclassOf<UAnimInstance> FP_CharacterAnimBlueprintClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    TSubclassOf<UAnimInstance> TP_CharacterAnimBlueprintClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "C_Animation")
    UAnimMontage* C_UnholsterAnimation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "C_Animation")
    UAnimMontage* C_FireAnimation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "C_Animation")
    UAnimMontage* C_FireAimedAnimation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "C_Animation")
    UAnimMontage* C_ReloadAnimation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "C_Animation")
    UAnimMontage* C_ReloadEmptyAnimation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "C_Animation")
    UAnimMontage* C_ViewAnimation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "W_Animation")
    UAnimMontage* W_UnholsterAnimation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "W_Animation")
    UAnimMontage* W_FireAnimation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "W_Animation")
    UAnimMontage* W_ReloadAnimation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "W_Animation")
    UAnimMontage* W_ReloadEmptyAnimation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TP_Animation")
    UAnimMontage* TP_FireAnimation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TP_Animation")
    UAnimMontage* TP_ReloadAnimation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TP_Animation")
    UAnimMontage* TP_ReloadEmptyAnimation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TP_Animation")
    UAnimMontage* TP_UnholsterAnimation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TP_Animation")
    UAnimMontage* TP_ViewAnimation;
};
