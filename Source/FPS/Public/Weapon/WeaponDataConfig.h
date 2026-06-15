#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WeaponRecoilPresets.h"
#include "WeaponDataConfig.generated.h"

/** 武器开火模式 */
UENUM(BlueprintType)
enum class EWeaponFireMode : uint8
{
	SemiAuto    UMETA(DisplayName = "Semi Auto"),
	FullAuto    UMETA(DisplayName = "Full Auto")
};

UCLASS(BlueprintType)
class FPS_API UWeaponDataConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	// ======================================================================
	// 后坐力
	// ======================================================================

	/** 后坐力图案预设 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil")
	EWeaponRecoilPattern PatternPreset = EWeaponRecoilPattern::None;

	/** 自定义后坐力图案（仅当 PatternPreset == Custom 时生效） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil",
		meta = (EditCondition = "PatternPreset == EWeaponRecoilPattern::Custom"))
	TArray<FVector2D> RecoilPattern;

	/** 停火多少发后图案索引归零 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil",
		meta = (ClampMin = "1", ClampMax = "50"))
	int32 PatternResetDelay = 8;

	/** 后坐力速度衰减率，越大衰减越快 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil",
		meta = (ClampMin = "3.0", ClampMax = "60.0"))
	float RecoilDecayRate = 15.0f;

	/** 后坐力强度倍率，越大越猛 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil",
		meta = (ClampMin = "10.0", ClampMax = "200.0"))
	float RecoilIntensity = 60.0f;

	// ======================================================================
	// 伤害
	// ======================================================================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage",
		meta = (ClampMin = "1.0", ClampMax = "200.0"))
	float BaseDamage = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage",
		meta = (ClampMin = "0.0", ClampMax = "50.0"))
	float DamageVariance = 5.f;

	// ======================================================================
	// 弹药
	// ======================================================================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ammo",
		meta = (ClampMin = "1", ClampMax = "200"))
	int32 MaxAmmo = 12;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ammo",
		meta = (ClampMin = "0", ClampMax = "5"))
	int32 DefaultMagazineNum = 2;

	// ======================================================================
	// 射击
	// ======================================================================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fire")
	EWeaponFireMode FireMode = EWeaponFireMode::SemiAuto;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fire",
		meta = (ClampMin = "0.05", ClampMax = "2.0"))
	float FireInterval = 0.17f;

	// ======================================================================
	// 扩散
	// ======================================================================

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

	// ======================================================================
	// 显示
	// ======================================================================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display")
	FString GunName;
};
