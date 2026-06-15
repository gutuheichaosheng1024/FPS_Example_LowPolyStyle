#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AIAttackConfig.generated.h"

/** AI 瞄准骨骼及命中概率 */
USTRUCT(BlueprintType)
struct FPS_API FAIAimBoneEntry
{
    GENERATED_BODY()

    /** 目标骨骼名（如 spine_03, head, pelvis） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
    FName BoneName;

    /** 命中该骨骼的概率 (0~1)，所有概率之和为 AI 总命中率 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI",
        meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float HitProbability = 0.0f;
};

UCLASS(BlueprintType)
class FPS_API UAIAttackConfig : public UDataAsset
{
    GENERATED_BODY()

public:
    /** 目标骨骼及对应命中概率 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
    TArray<FAIAimBoneEntry> AimBones;

    /** 未命中时子弹偏离准星最大角度（度） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI",
        meta = (ClampMin = "5.0", ClampMax = "90.0"))
    float MissConeAngle = 30.0f;

    /** 未命中时最小偏角（度），确保子弹不擦边命中目标 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI",
        meta = (ClampMin = "2.0", ClampMax = "90.0"))
    float MinMissAngle = 5.0f;

    /** 直接开火的距离阈值（cm），≤此距离 + 无遮挡 → 立即开火 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Distance",
        meta = (ClampMin = "100", ClampMax = "10000"))
    float FireRange = 1500.f;
};
