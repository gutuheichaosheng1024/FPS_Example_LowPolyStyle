#pragma once

#include "CoreMinimal.h"

class UWeaponDataConfig;
class AFPS_CharacterBase;

/**
 * FWeaponRecoilHandler — 武器后坐力处理器
 *
 * 职责：管理后坐力图案索引、速度衰减模型和相机输入驱动，根据 DataConfig 中的图案预设或自定义图案计算每发子弹的后坐力冲量，逐帧衰减并应用到玩家相机
 * 使用：AFPS_CharacterBase（持有者角色）、UWeaponDataConfig（后坐力数值配置）、FPS::RecoilPresets（图案预设数据）
 */
struct FWeaponRecoilHandler
{
public:
    FWeaponRecoilHandler();

    void Init(AFPS_CharacterBase* InCharacter);
    void ClearCharacter();
    void UpdateRecoil(float DeltaTime, const UWeaponDataConfig* Config);
    void ApplyRecoil(const UWeaponDataConfig* Config);
    void Reset();

private:
    void ApplyRecoilToCamera(float PitchDelta, float YawDelta);

    AFPS_CharacterBase* Character = nullptr;

    int32 PatternIndex = 0;
    int32 ShotsSinceLastRecoil = 0;

    float VelocityPitch = 0.f;
    float VelocityYaw = 0.f;
};
