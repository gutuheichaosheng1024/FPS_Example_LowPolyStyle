#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AIAttackConfig.generated.h"

/**
 * FAIAimBoneEntry — AI 瞄准骨骼条目
 *
 * 职责：定义 AI 瞄准的目标骨骼及对应命中概率，所有骨骼概率之和即为 AI 总命中率
 * 使用：UAIAttackConfig（父配置类）
 */
USTRUCT(BlueprintType)
struct FPS_API FAIAimBoneEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
    FName BoneName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI",
        meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float HitProbability = 0.0f;
};

/**
 * UAIAttackConfig — AI 攻击参数配置 DataAsset
 *
 * 职责：配置 AI 的攻击行为参数，包括瞄准骨骼分布、未命中偏离角度和开火距离阈值
 * 使用：FAIAimBoneEntry（瞄准骨骼条目）
 */
UCLASS(BlueprintType)
class FPS_API UAIAttackConfig : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
    TArray<FAIAimBoneEntry> AimBones;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI",
        meta = (ClampMin = "5.0", ClampMax = "90.0"))
    float MissConeAngle = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI",
        meta = (ClampMin = "2.0", ClampMax = "90.0"))
    float MinMissAngle = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Distance",
        meta = (ClampMin = "100", ClampMax = "10000"))
    float FireRange = 1500.f;
};
