#pragma once

#include "CoreMinimal.h"
#include "WeaponRecoilPresets.generated.h"

UENUM(BlueprintType)
enum class EWeaponRecoilPattern : uint8
{
    None        UMETA(DisplayName = "None"),
    AK47        UMETA(DisplayName = "AK47"),
    M4A1        UMETA(DisplayName = "M4A1"),
    Pistol      UMETA(DisplayName = "Pistol"),
    SMG         UMETA(DisplayName = "SMG"),
    LMG         UMETA(DisplayName = "LMG"),
    Sniper      UMETA(DisplayName = "Sniper"),
    Custom      UMETA(DisplayName = "Custom"),
};

namespace FPS::RecoilPresets
{
    inline const TArray<FVector2D> AK47 = {
        { 0.60f,  0.10f }, { 0.65f,  0.20f }, { 0.70f,  0.30f }, { 0.75f,  0.40f }, { 0.80f,  0.50f },
        { 0.70f,  0.55f }, { 0.65f,  0.60f }, { 0.60f,  0.55f }, { 0.55f,  0.50f }, { 0.60f,  0.60f },
        { 0.65f,  0.65f }, { 0.60f,  0.55f }, { 0.55f,  0.65f }, { 0.60f,  0.70f }, { 0.55f,  0.60f },
        { 0.50f,  0.55f }, { 0.55f,  0.65f }, { 0.60f,  0.70f }, { 0.55f,  0.60f }, { 0.50f,  0.70f },
        { 0.55f,  0.75f }, { 0.50f,  0.65f }, { 0.45f,  0.70f }, { 0.50f,  0.60f }, { 0.55f,  0.65f },
        { 0.50f,  0.55f }, { 0.45f,  0.65f }, { 0.50f,  0.70f }, { 0.45f,  0.60f }, { 0.50f,  0.55f },
    };

    inline const TArray<FVector2D> M4A1 = {
        { 0.40f,  0.06f }, { 0.42f,  0.10f }, { 0.45f,  0.12f }, { 0.48f,  0.15f }, { 0.50f,  0.12f },
        { 0.48f,  0.18f }, { 0.45f,  0.15f }, { 0.42f,  0.12f }, { 0.45f,  0.18f }, { 0.48f,  0.15f },
        { 0.45f,  0.12f }, { 0.42f,  0.18f }, { 0.45f,  0.15f }, { 0.48f,  0.12f }, { 0.45f,  0.18f },
        { 0.42f,  0.15f }, { 0.40f,  0.12f }, { 0.42f,  0.18f }, { 0.45f,  0.15f }, { 0.42f,  0.12f },
        { 0.40f,  0.18f }, { 0.42f,  0.15f }, { 0.40f,  0.12f }, { 0.38f,  0.18f }, { 0.40f,  0.15f },
        { 0.42f,  0.12f }, { 0.40f,  0.18f }, { 0.38f,  0.15f }, { 0.40f,  0.12f }, { 0.42f,  0.15f },
    };

    inline const TArray<FVector2D> Pistol = {
        { 1.20f,  0.15f }, { 1.30f,  0.25f }, { 1.40f,  0.35f }, { 1.50f,  0.45f },
        { 1.35f,  0.35f }, { 1.20f,  0.25f }, { 1.30f,  0.45f }, { 1.40f,  0.35f },
        { 1.25f,  0.25f }, { 1.15f,  0.35f }, { 1.20f,  0.45f }, { 1.30f,  0.25f },
    };

    inline const TArray<FVector2D> SMG = {
        { 0.25f,  0.15f }, { 0.28f, -0.25f }, { 0.30f,  0.30f }, { 0.32f, -0.20f }, { 0.30f,  0.35f },
        { 0.28f, -0.30f }, { 0.25f,  0.25f }, { 0.30f, -0.35f }, { 0.32f,  0.20f }, { 0.28f, -0.25f },
        { 0.25f,  0.30f }, { 0.30f, -0.15f }, { 0.28f,  0.35f }, { 0.32f, -0.30f }, { 0.30f,  0.25f },
        { 0.25f, -0.35f }, { 0.28f,  0.20f }, { 0.30f, -0.25f }, { 0.32f,  0.30f }, { 0.28f, -0.15f },
        { 0.25f,  0.35f }, { 0.30f, -0.30f }, { 0.28f,  0.25f }, { 0.25f, -0.35f }, { 0.30f,  0.20f },
        { 0.28f, -0.25f }, { 0.32f,  0.30f }, { 0.30f, -0.15f }, { 0.28f,  0.35f }, { 0.25f, -0.25f },
    };

    inline const TArray<FVector2D> LMG = {
        { 0.40f,  0.06f }, { 0.42f,  0.12f }, { 0.45f,  0.10f }, { 0.48f,  0.15f }, { 0.50f,  0.12f },
        { 0.48f,  0.18f }, { 0.45f,  0.15f }, { 0.50f,  0.12f }, { 0.48f,  0.18f }, { 0.45f,  0.15f },
        { 0.55f,  0.25f }, { 0.60f,  0.30f }, { 0.65f,  0.35f }, { 0.60f,  0.45f }, { 0.55f,  0.55f },
        { 0.60f,  0.60f }, { 0.65f,  0.45f }, { 0.70f,  0.55f }, { 0.65f,  0.65f }, { 0.60f,  0.60f },
        { 0.70f,  0.75f }, { 0.75f,  0.60f }, { 0.80f,  0.85f }, { 0.75f,  0.90f }, { 0.70f,  0.75f },
        { 0.75f,  0.65f }, { 0.80f,  0.85f }, { 0.85f,  0.95f }, { 0.80f,  0.75f }, { 0.75f,  0.90f },
        { 0.85f,  1.05f }, { 0.90f,  0.85f }, { 0.95f,  1.15f }, { 0.90f,  1.20f }, { 0.85f,  0.95f },
        { 0.95f,  1.05f }, { 1.00f,  1.25f }, { 0.95f,  1.15f }, { 0.90f,  1.35f }, { 0.95f,  1.20f },
        { 1.00f,  1.45f }, { 1.05f,  1.25f }, { 1.10f,  1.50f }, { 1.05f,  1.65f }, { 1.00f,  1.45f },
        { 1.10f,  1.55f }, { 1.15f,  1.75f }, { 1.10f,  1.50f }, { 1.05f,  1.65f }, { 1.00f,  1.80f },
    };

    inline const TArray<FVector2D> Sniper = {
        { 3.50f,  0.30f },
    };

    inline const TArray<FVector2D>* GetPattern(const EWeaponRecoilPattern Preset)
    {
        switch (Preset)
        {
        case EWeaponRecoilPattern::AK47:   return &AK47;
        case EWeaponRecoilPattern::M4A1:   return &M4A1;
        case EWeaponRecoilPattern::Pistol: return &Pistol;
        case EWeaponRecoilPattern::SMG:    return &SMG;
        case EWeaponRecoilPattern::LMG:    return &LMG;
        case EWeaponRecoilPattern::Sniper: return &Sniper;
        default:                           return nullptr;
        }
    }
}
