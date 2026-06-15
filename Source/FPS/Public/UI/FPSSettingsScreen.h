#pragma once

#include "CoreMinimal.h"
#include "UI/FPSBaseMenuWidget.h"
#include "FPSSettingsScreen.generated.h"

class USlider;
class UTextBlock;
class UComboBoxString;
class UCheckBox;
class UButton;

/**
 * 设置界面
 * 音量、分辨率、全屏设置，支持实时预览和持久化保存
 */
UCLASS(Abstract)
class FPS_API UFPSSettingsScreen : public UFPSBaseMenuWidget
{
    GENERATED_BODY()

protected:
    // ---------- 音量 ----------
    UPROPERTY(meta = (BindWidget))
    USlider* MasterVolumeSlider;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* MasterVolumeText;

    UPROPERTY(meta = (BindWidget))
    USlider* BackgroundVolumeSlider;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* BackgroundVolumeText;

    // ---------- 分辨率 ----------
    UPROPERTY(meta = (BindWidget))
    UComboBoxString* ResolutionComboBox;

    UPROPERTY(meta = (BindWidget))
    UCheckBox* FullscreenCheckBox;

    // ---------- 按钮 ----------
    UPROPERTY(meta = (BindWidget))
    UButton* BackButton;

    virtual void NativeConstruct() override;

private:
    /** 加载并应用保存的设置 */
    void LoadAndApplySettings();

    /** 获取支持的分辨率列表 */
    TArray<FString> GetSupportedResolutions();

    /** 应用分辨率和全屏设置 */
    void ApplyResolution(const FString& Res, bool bFullscreen);

    // ---------- 事件回调 ----------
    UFUNCTION()
    void OnMasterVolumeChanged(float Value);

    UFUNCTION()
    void OnBackgroundVolumeChanged(float Value);

    UFUNCTION()
    void OnResolutionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

    UFUNCTION()
    void OnFullscreenChanged(bool bIsChecked);

    UFUNCTION()
    void OnBackClicked();
};
