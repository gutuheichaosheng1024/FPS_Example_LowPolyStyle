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

// 重置图案索引、未开火计数和后坐力速度
// 流程：将 PatternIndex、ShotsSinceLastRecoil、VelocityPitch、VelocityYaw 全部归零
void FWeaponRecoilHandler::Reset()
{
    PatternIndex = 0;
    ShotsSinceLastRecoil = 0;
    VelocityPitch = 0.f;
    VelocityYaw = 0.f;
}

// 开火时应用后坐力：从图案读取冲量，累加到速度变量
// 流程：检查角色是否为玩家控制 → 解析图案来源（Custom/预设/无）→ 取当前索引的图案值 × 强度 → 累加 Pitch/Yaw 速度 → 递增图案索引 → 重置未开火计数
void FWeaponRecoilHandler::ApplyRecoil(const UWeaponDataConfig* Config)
{
    if (!Config || !Character) return;

    if (!Character->IsPlayerControlled()) return;

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
        VelocityPitch += -Impulse.X * Config->RecoilIntensity;
        VelocityYaw   +=  Impulse.Y * Config->RecoilIntensity;
        PatternIndex = FMath::Min(PatternIndex + 1, PatternNum - 1);
    }

    ShotsSinceLastRecoil = 0;
}

// 每帧更新后坐力：速度衰减 + 应用相机输入 + 图案索引归零检测
// 流程：检查速度是否为零 → 将速度×dt 应用到相机 → 按衰减率缩减速度 → 递增未开火计数 → 若超过重置延迟则将图案索引归零
void FWeaponRecoilHandler::UpdateRecoil(float DeltaTime, const UWeaponDataConfig* Config)
{
    if (!Config) return;

    if (FMath::Abs(VelocityPitch) > KINDA_SMALL_NUMBER ||
        FMath::Abs(VelocityYaw)   > KINDA_SMALL_NUMBER)
    {
        ApplyRecoilToCamera(VelocityPitch * DeltaTime, VelocityYaw * DeltaTime);

        const float Drag = FMath::Clamp(Config->RecoilDecayRate * DeltaTime, 0.f, 1.f);
        VelocityPitch *= (1.f - Drag);
        VelocityYaw   *= (1.f - Drag);
    }

    if (ShotsSinceLastRecoil == 0)
        ShotsSinceLastRecoil = 1;
    else
        ShotsSinceLastRecoil++;

    if (ShotsSinceLastRecoil > Config->PatternResetDelay)
    {
        PatternIndex = 0;
    }
}

// 将后坐力速度增量应用到玩家相机
// 流程：获取 PlayerController → 调用 AddPitchInput/AddYawInput 驱动相机旋转
void FWeaponRecoilHandler::ApplyRecoilToCamera(float PitchDelta, float YawDelta)
{
    if (!Character) return;
    APlayerController* PC = Cast<APlayerController>(Character->GetController());
    if (!PC) return;

    PC->AddPitchInput(PitchDelta);
    PC->AddYawInput(YawDelta);
}
