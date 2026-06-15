#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WeaponAnimationConfig.generated.h"

class UAnimInstance;
class UAnimMontage;

UCLASS(BlueprintType)
class FPS_API UWeaponAnimationConfig : public UDataAsset
{
    GENERATED_BODY()

public:
    // ---------- AnimBlueprint 类 ----------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    TSubclassOf<UAnimInstance> FP_CharacterAnimBlueprintClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    TSubclassOf<UAnimInstance> TP_CharacterAnimBlueprintClass;

    // ---------- C_ 前缀：角色 FP Montage ----------
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

    // ---------- W_ 前缀：武器 FP Montage ----------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "W_Animation")
    UAnimMontage* W_UnholsterAnimation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "W_Animation")
    UAnimMontage* W_FireAnimation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "W_Animation")
    UAnimMontage* W_ReloadAnimation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "W_Animation")
    UAnimMontage* W_ReloadEmptyAnimation;

    // ---------- TP_ 前缀：第三人称 Montage ----------
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
