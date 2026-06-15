#pragma once

#include "CoreMinimal.h"

class UWeaponDataConfig;
class AFPS_CharacterBase;

/**
 * 武器扩散处理器
 * 管理移动扩散、射击累积扩散、总扩散角度计算
 * 由 WeaponActor 持有，配置在调用时传入
 */
struct FWeaponSpreadHandler
{
public:
    FWeaponSpreadHandler();

    /** 绑定持有者角色（Equip 时调用） */
    void Init(AFPS_CharacterBase* InCharacter);

    /** 清除持有者（Drop 时调用） */
    void ClearCharacter();

    /** 每帧更新：射击扩散衰减 */
    void UpdateSpread(float DeltaTime, const UWeaponDataConfig* Config);

    /** 开火时调用：累积射击扩散（仅玩家，AI 跳过） */
    void AddSpreadOnFire(const UWeaponDataConfig* Config);

    /** 获取当前总扩散角度（移动 + 射击，含 AI 瞄准修正） */
    float GetTotalSpreadAngle(const UWeaponDataConfig* Config) const;

private:
    /** 移动扩散角度 */
    float GetMoveSpreadAngle(const UWeaponDataConfig* Config) const;

    /** 射击累积扩散角度 */
    float GetShootSpreadAngle() const;

    // 持有者
    AFPS_CharacterBase* Character = nullptr;

    // 射击扩散状态
    float SpreadReturnStartValue = 0.f;
    float TimeSinceLastShot = 0.f;
    float CurrentShootSpread = 0.f;
};
