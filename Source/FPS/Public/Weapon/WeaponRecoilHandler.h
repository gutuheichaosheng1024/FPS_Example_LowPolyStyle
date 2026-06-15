#pragma once

#include "CoreMinimal.h"

class UWeaponDataConfig;
class AFPS_CharacterBase;

/**
 * 武器后坐力处理器
 * 管理后坐力图案索引、速度衰减、相机输入驱动
 * 由 WeaponActor 持有，配置在调用时传入
 */
struct FWeaponRecoilHandler
{
public:
    FWeaponRecoilHandler();

    /** 绑定持有者角色（Equip/Activate 时调用） */
    void Init(AFPS_CharacterBase* InCharacter);

    /** 清除持有者（Deactivate/Drop 时调用） */
    void ClearCharacter();

    /** 每帧更新：速度衰减 + 应用相机输入 */
    void UpdateRecoil(float DeltaTime, const UWeaponDataConfig* Config);

    /** 开火时调用：从图案读取冲量，驱动相机后坐力 */
    void ApplyRecoil(const UWeaponDataConfig* Config);

    /** 重置图案索引和速度（换弹/切换武器时调用） */
    void Reset();

private:
    /** 将后坐力速度应用到相机 */
    void ApplyRecoilToCamera(float PitchDelta, float YawDelta);

    // 持有者
    AFPS_CharacterBase* Character = nullptr;

    // 图案状态
    int32 PatternIndex = 0;
    int32 ShotsSinceLastRecoil = 0;

    // 后坐力速度（度/秒），衰减模型
    float VelocityPitch = 0.f;
    float VelocityYaw = 0.f;
};
