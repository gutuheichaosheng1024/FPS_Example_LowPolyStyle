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

void FWeaponSpreadHandler::AddSpreadOnFire(const UWeaponDataConfig* Config)
{
    if (!Config) return;
    // AI 不累积射击扩散（无相机后坐力，累积会导致精度无限恶化）
    if (Character && !Character->IsPlayerControlled()) return;
    CurrentShootSpread = FMath::Clamp(CurrentShootSpread + Config->ShootSpreadPerShot, Config->ShootSpreadMin, Config->ShootSpreadMax);
    SpreadReturnStartValue = CurrentShootSpread;
    TimeSinceLastShot = 0.f;
}

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
