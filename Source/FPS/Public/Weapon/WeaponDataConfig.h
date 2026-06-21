#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WeaponRecoilPresets.h"
#include "WeaponDataConfig.generated.h"

UENUM(BlueprintType)
enum class EWeaponFireMode : uint8
{
    SemiAuto    UMETA(DisplayName = "Semi Auto"),
    FullAuto    UMETA(DisplayName = "Full Auto")
};

/**
 * UWeaponDataConfig — 武器数值配置 DataAsset
 *
 * 职责：配置武器的核心战斗数值，包括后坐力（图案预设/强度/衰减）、伤害（基础值/浮动）、弹药（弹匣/备弹）、射击（模式/间隔）和扩散（移动/射击/AI修正）
 * 使用：EWeaponRecoilPattern（后坐力图案枚举）、EWeaponFireMode（开火模式枚举）
 */
UCLASS(BlueprintType)
class FPS_API UWeaponDataConfig : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil")
    EWeaponRecoilPattern PatternPreset = EWeaponRecoilPattern::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil",
        meta = (EditCondition = "PatternPreset == EWeaponRecoilPattern::Custom"))
    TArray<FVector2D> RecoilPattern;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil",
        meta = (ClampMin = "1", ClampMax = "50"))
    int32 PatternResetDelay = 8;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil",
        meta = (ClampMin = "3.0", ClampMax = "60.0"))
    float RecoilDecayRate = 15.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil",
        meta = (ClampMin = "10.0", ClampMax = "200.0"))
    float RecoilIntensity = 60.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage",
        meta = (ClampMin = "1.0", ClampMax = "200.0"))
    float BaseDamage = 20.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage",
        meta = (ClampMin = "0.0", ClampMax = "50.0"))
    float DamageVariance = 5.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ammo",
        meta = (ClampMin = "1", ClampMax = "200"))
    int32 MaxAmmo = 12;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ammo",
        meta = (ClampMin = "0", ClampMax = "5"))
    int32 DefaultMagazineNum = 2;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fire")
    EWeaponFireMode FireMode = EWeaponFireMode::SemiAuto;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fire",
        meta = (ClampMin = "0.05", ClampMax = "2.0"))
    float FireInterval = 0.17f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spread|Move",
        meta = (ClampMin = "0.0", ClampMax = "10.0"))
    float MoveSpreadMin = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spread|Move",
        meta = (ClampMin = "0.0", ClampMax = "45.0"))
    float MoveSpreadMax = 20.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spread|Move",
        meta = (ClampMin = "1.0", ClampMax = "1200.0"))
    float MoveSpreadSpeedScale = 700.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spread|Shoot",
        meta = (ClampMin = "0.0", ClampMax = "5.0"))
    float ShootSpreadMin = 0.2f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spread|Shoot",
        meta = (ClampMin = "0.0", ClampMax = "45.0"))
    float ShootSpreadMax = 15.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spread|Shoot",
        meta = (ClampMin = "0.0", ClampMax = "5.0"))
    float ShootSpreadPerShot = 2.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spread|Shoot",
        meta = (ClampMin = "0.01", ClampMax = "1.0"))
    float ShootSpreadResetTime = 0.4f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spread",
        meta = (ClampMin = "1.0", ClampMax = "10.0"))
    float AISpreadMultiplier = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display")
    FString GunName;
};
