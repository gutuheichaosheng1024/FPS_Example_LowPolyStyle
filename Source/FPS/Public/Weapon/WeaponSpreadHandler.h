#pragma once

#include "CoreMinimal.h"

class UWeaponDataConfig;
class AFPS_CharacterBase;

/**
 * FWeaponSpreadHandler — 武器扩散处理器
 *
 * 职责：管理移动扩散（基于角色速度）和射击累积扩散（逐发递增+衰减），计算总扩散角度并考虑瞄准减半和 AI 倍率修正
 * 使用：AFPS_CharacterBase（持有者角色）、UWeaponDataConfig（扩散数值配置）
 */
struct FWeaponSpreadHandler
{
public:
    FWeaponSpreadHandler();

    void Init(AFPS_CharacterBase* InCharacter);
    void ClearCharacter();
    void UpdateSpread(float DeltaTime, const UWeaponDataConfig* Config);
    void AddSpreadOnFire(const UWeaponDataConfig* Config);
    float GetTotalSpreadAngle(const UWeaponDataConfig* Config) const;

private:
    float GetMoveSpreadAngle(const UWeaponDataConfig* Config) const;
    float GetShootSpreadAngle() const;

    AFPS_CharacterBase* Character = nullptr;

    float SpreadReturnStartValue = 0.f;
    float TimeSinceLastShot = 0.f;
    float CurrentShootSpread = 0.f;
};
