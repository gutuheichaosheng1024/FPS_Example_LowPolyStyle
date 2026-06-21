#include "Weapon/WeaponSpreadHandler.h"
#include "Weapon/WeaponDataConfig.h"
#include "Character/FPS_CharacterBase.h"

FWeaponSpreadHandler::FWeaponSpreadHandler()
{
}

void FWeaponSpreadHandler::Init(AFPS_CharacterBase* InCharacter)
{
    Character = InCharacter;
    CurrentShootSpread = 0.f;
    SpreadReturnStartValue = 0.f;
    TimeSinceLastShot = 0.f;
}

void FWeaponSpreadHandler::ClearCharacter()
{
    Character = nullptr;
    CurrentShootSpread = 0.f;
    SpreadReturnStartValue = 0.f;
    TimeSinceLastShot = 0.f;
}

// 每帧更新射击扩散：基于时间渐进衰减到最小值
// 流程：检查 ResetTime 是否有效 → 累加距上次射击的时间 → 计算衰减比例 Alpha → 从 ReturnStartValue 插值到 ShootSpreadMin
void FWeaponSpreadHandler::UpdateSpread(float DeltaTime, const UWeaponDataConfig* Config)
{
    if (!Config) return;
    if (Config->ShootSpreadResetTime <= KINDA_SMALL_NUMBER)
    {
        CurrentShootSpread = Config->ShootSpreadMin;
        return;
    }
    TimeSinceLastShot += DeltaTime;
    float Alpha = FMath::Clamp(TimeSinceLastShot / Config->ShootSpreadResetTime, 0.f, 1.f);
    CurrentShootSpread = FMath::Lerp(SpreadReturnStartValue, Config->ShootSpreadMin, Alpha);
}

// 开火时累积射击扩散
// 流程：跳过 AI（AI 不累积射击扩散）→ 当前扩散 += ShootSpreadPerShot → 钳制到 Min/Max 范围 → 记录 ReturnStartValue → 重置计时器
void FWeaponSpreadHandler::AddSpreadOnFire(const UWeaponDataConfig* Config)
{
    if (!Config) return;
    if (Character && !Character->IsPlayerControlled()) return;
    CurrentShootSpread = FMath::Clamp(CurrentShootSpread + Config->ShootSpreadPerShot, Config->ShootSpreadMin, Config->ShootSpreadMax);
    SpreadReturnStartValue = CurrentShootSpread;
    TimeSinceLastShot = 0.f;
}

// 计算当前总扩散角度
// 流程：取移动扩散 → 玩家叠加射击扩散 → 瞄准时减半 → AI 乘以 AISpreadMultiplier → 返回最终角度
float FWeaponSpreadHandler::GetTotalSpreadAngle(const UWeaponDataConfig* Config) const
{
    if (!Config) return 0.f;
    float Total = GetMoveSpreadAngle(Config);
    if (Character && Character->IsPlayerControlled())
        Total += GetShootSpreadAngle();
    if (Character && Character->Aiming)
        Total *= 0.5f;
    if (Character && !Character->IsPlayerControlled())
        Total *= Config->AISpreadMultiplier;
    return Total;
}

// 计算移动扩散角度
// 流程：取角色速度大小 → 计算速度因子（速度/MoveSpreadSpeedScale 钳制到 0~1）→ 在 MoveSpreadMin 和 MoveSpreadMax 之间插值
float FWeaponSpreadHandler::GetMoveSpreadAngle(const UWeaponDataConfig* Config) const
{
    if (!Character || !Config) return 0.f;
    float Speed = Character->GetVelocity().Size();
    float Factor = FMath::Clamp(Speed / Config->MoveSpreadSpeedScale, 0.f, 1.f);
    return FMath::Lerp(Config->MoveSpreadMin, Config->MoveSpreadMax, Factor);
}

float FWeaponSpreadHandler::GetShootSpreadAngle() const
{
    return CurrentShootSpread;
}
