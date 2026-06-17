#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "UI/FPSCrosshairWidget.h"
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

    /** 玩家用户名 */
    UPROPERTY()
    FString PlayerName = TEXT("Player");

    // ---------- 准星配置 ----------
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

    // ---------- 存档常量 ----------
    static const FString SaveSlotName;
    static const int32 UserIndex;

    /** 加载设置，不存在则返回默认值 */
    UFUNCTION(BlueprintCallable, Category = "Settings")
    static UFPSSaveGame* LoadSettings();

    /** 保存设置（保留已存储的 PlayerName） */
    UFUNCTION(BlueprintCallable, Category = "Settings")
    static void SaveSettings(float Master, float Background, const FString& Res, bool bInFullscreen);

    /** 读取用户名，不存在返回 "Player" */
    UFUNCTION(BlueprintCallable, Category = "Settings")
    static FString LoadPlayerName();

    /** 保存用户名（保留已存储的其他设置） */
    UFUNCTION(BlueprintCallable, Category = "Settings")
    static void SavePlayerName(const FString& Name);

    /** 保存准星配置 */
    UFUNCTION(BlueprintCallable, Category = "Settings")
    static void SaveCrosshairConfig(const FCrosshairConfig& Config);

    /** 加载准星配置，不存在返回默认值 */
    UFUNCTION(BlueprintCallable, Category = "Settings")
    static FCrosshairConfig LoadCrosshairConfig();
};
