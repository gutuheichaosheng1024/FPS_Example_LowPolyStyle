#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "FPSSaveGame.generated.h"

/**
 * 游戏设置持久化
 * 存储音量、分辨率、全屏等设置到本地存档
 */
UCLASS()
class FPS_API UFPSSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY()
    float MasterVolume = 1.0f;

    UPROPERTY()
    float BackgroundVolume = 0.5f;

    UPROPERTY()
    FString Resolution = TEXT("1920x1080");

    UPROPERTY()
    bool bFullscreen = true;

    static const FString SaveSlotName;
    static const int32 UserIndex;

    /** 加载设置，不存在则返回默认值 */
    UFUNCTION(BlueprintCallable, Category = "Settings")
    static UFPSSaveGame* LoadSettings();

    /** 保存设置 */
    UFUNCTION(BlueprintCallable, Category = "Settings")
    static void SaveSettings(float Master, float Background, const FString& Res, bool bInFullscreen);
};
