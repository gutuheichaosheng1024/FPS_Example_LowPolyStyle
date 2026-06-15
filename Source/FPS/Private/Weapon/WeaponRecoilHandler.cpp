#include "Weapon/WeaponRecoilHandler.h"
#include "Weapon/WeaponDataConfig.h"
#include "Weapon/WeaponRecoilPresets.h"
#include "Character/FPS_CharacterBase.h"
#include "GameFramework/PlayerController.h"

FWeaponRecoilHandler::FWeaponRecoilHandler()
{
}

void FWeaponRecoilHandler::Init(AFPS_CharacterBase* InCharacter)
{
    Character = InCharacter;
    Reset();
}

void FWeaponRecoilHandler::ClearCharacter()
{
    Character = nullptr;
    Reset();
}

void FWeaponRecoilHandler::Reset()
{
    PatternIndex = 0;
    ShotsSinceLastRecoil = 0;
    VelocityPitch = 0.f;
    VelocityYaw = 0.f;
}

void FWeaponRecoilHandler::ApplyRecoil(const UWeaponDataConfig* Config)
{
    if (!Config || !Character) return;

    // 仅玩家控制的角色应用图案后坐力
    if (!Character->IsPlayerControlled()) return;

    // 解析图案来源
    const TArray<FVector2D>* Pattern = nullptr;
    int32 PatternNum = 0;

    if (Config->PatternPreset == EWeaponRecoilPattern::Custom)
    {
        Pattern = &Config->RecoilPattern;
        PatternNum = Config->RecoilPattern.Num();
    }
    else if (Config->PatternPreset != EWeaponRecoilPattern::None)
    {
        Pattern = FPS::RecoilPresets::GetPattern(Config->PatternPreset);
        PatternNum = Pattern ? Pattern->Num() : 0;
    }

    if (Pattern && PatternNum > 0)
    {
        const int32 Idx = FMath::Min(PatternIndex, PatternNum - 1);
        const FVector2D Impulse = (*Pattern)[Idx];
        // 速度冲量 = 图案值 * 强度（Pitch 取反，图案定义为准星上移量）
        VelocityPitch += -Impulse.X * Config->RecoilIntensity;
        VelocityYaw   +=  Impulse.Y * Config->RecoilIntensity;
        PatternIndex = FMath::Min(PatternIndex + 1, PatternNum - 1);
    }

    ShotsSinceLastRecoil = 0;
}

void FWeaponRecoilHandler::UpdateRecoil(float DeltaTime, const UWeaponDataConfig* Config)
{
    if (!Config) return;

    // 速度衰减模型：每帧应用 velocity*dt 到相机，然后 velocity *= (1 - decay*dt)
    if (FMath::Abs(VelocityPitch) > KINDA_SMALL_NUMBER ||
        FMath::Abs(VelocityYaw)   > KINDA_SMALL_NUMBER)
    {
        ApplyRecoilToCamera(VelocityPitch * DeltaTime, VelocityYaw * DeltaTime);

        const float Drag = FMath::Clamp(Config->RecoilDecayRate * DeltaTime, 0.f, 1.f);
        VelocityPitch *= (1.f - Drag);
        VelocityYaw   *= (1.f - Drag);
    }

    // 图案重置：连续 N 发未开火则归零
    if (ShotsSinceLastRecoil == 0)
        ShotsSinceLastRecoil = 1;
    else
        ShotsSinceLastRecoil++;

    if (ShotsSinceLastRecoil > Config->PatternResetDelay)
    {
        PatternIndex = 0;
    }
}

void FWeaponRecoilHandler::ApplyRecoilToCamera(float PitchDelta, float YawDelta)
{
    if (!Character) return;
    APlayerController* PC = Cast<APlayerController>(Character->GetController());
    if (!PC) return;

    PC->AddPitchInput(PitchDelta);
    PC->AddYawInput(YawDelta);
}
