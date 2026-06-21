#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "UI/FPSCrosshairWidget.h"
#include "FPSSaveGame.generated.h"

/**
 * UFPSSaveGame — 游戏设置持久化存储，保存音量、分辨率、全屏、玩家名称和准星配置到本地存档
 *
 * 职责：提供 LoadSettings/SaveSettings 管理通用设置（音量/分辨率/全屏/玩家名）；提供 SaveCrosshairConfig/LoadCrosshairConfig 管理准星配置的独立存档；静态方法可跨界面直接调用
 * 使用：USaveGame、UGameplayStatics、FCrosshairConfig
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

    UPROPERTY()
    FString PlayerName = TEXT("Player");

    UPROPERTY()
    int32 CrosshairShape = 2;

    UPROPERTY()
    bool CrosshairShowCenterDot = true;

    UPROPERTY()
    float CrosshairCenterDotRadius = 2.f;

    UPROPERTY()
    FLinearColor CrosshairCenterDotColor = FLinearColor::White;

    UPROPERTY()
    float CrosshairCircleRadius = 14.f;

    UPROPERTY()
    float CrosshairCircleThickness = 2.f;

    UPROPERTY()
    bool CrosshairCircleFilled = false;

    UPROPERTY()
    float CrosshairBarLength = 14.f;

    UPROPERTY()
    float CrosshairBarThickness = 3.f;

    UPROPERTY()
    float CrosshairBarGap = 6.f;

    UPROPERTY()
    FLinearColor CrosshairColor = FLinearColor(0.f, 1.f, 0.f, 0.9f);

    UPROPERTY()
    float CrosshairOverallScale = 1.f;

    static const FString SaveSlotName;
    static const int32 UserIndex;

    UFUNCTION(BlueprintCallable, Category = "Settings")
    static UFPSSaveGame* LoadSettings();

    UFUNCTION(BlueprintCallable, Category = "Settings")
    static void SaveSettings(float Master, float Background, const FString& Res, bool bInFullscreen);

    UFUNCTION(BlueprintCallable, Category = "Settings")
    static FString LoadPlayerName();

    UFUNCTION(BlueprintCallable, Category = "Settings")
    static void SavePlayerName(const FString& Name);

    UFUNCTION(BlueprintCallable, Category = "Settings")
    static void SaveCrosshairConfig(const FCrosshairConfig& Config);

    UFUNCTION(BlueprintCallable, Category = "Settings")
    static FCrosshairConfig LoadCrosshairConfig();
};
