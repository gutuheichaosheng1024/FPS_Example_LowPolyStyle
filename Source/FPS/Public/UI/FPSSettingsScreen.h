#pragma once

#include "CoreMinimal.h"
#include "UI/FPSBaseMenuWidget.h"
#include "UI/FPSCrosshairWidget.h"
#include "FPSSettingsScreen.generated.h"

class USlider;
class UTextBlock;
class UComboBoxString;
class UCheckBox;
class UButton;
class UOverlay;
class UWidget;
class UFPSCrosshairWidget;

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

    // ---------- 页面导航 ----------
    UPROPERTY(meta = (BindWidget))
    UButton* GeneralSettingsButton;

    UPROPERTY(meta = (BindWidget))
    UButton* CrosshairSettingsButton;

    UPROPERTY(meta = (BindWidget))
    UOverlay* SettingsPageContainer;

    UPROPERTY()
    TObjectPtr<UWidget> GeneralPage;

    UPROPERTY()
    TObjectPtr<UWidget> CrosshairPage;

    // ---------- 准星形状 ----------
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UComboBoxString> CrosshairShapeCombo;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UCheckBox> ShowCenterDotCheckBox;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<USlider> CenterDotRadiusSlider;

    // ---------- 四角参数 ----------
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<USlider> BarLengthSlider;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<USlider> BarThicknessSlider;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<USlider> BarGapSlider;

    // ---------- 圆形参数 ----------
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<USlider> CircleRadiusSlider;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<USlider> CircleThicknessSlider;

    // ---------- 缩放 ----------
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<USlider> CrosshairScaleSlider;

    // ---------- 颜色（Button → FColorPicker）----------
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> CrosshairColorButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> CenterDotColorButton;

    // ---------- 形状参数区包裹（控制可见性）----------
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UWidget> FourCornerParams;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UWidget> CircleParams;

    // ---------- 预览 ----------
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UFPSCrosshairWidget> PreviewCrosshair;

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

    // ---------- 准星设置回调 ----------
    UFUNCTION()
    void OnGeneralSettingsClicked();

    UFUNCTION()
    void OnCrosshairSettingsClicked();

    UFUNCTION()
    void OnAnyCrosshairParamChanged(float Value);

    UFUNCTION()
    void OnCrosshairCheckChanged(bool bChecked);

    UFUNCTION()
    void OnCrosshairShapeChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

    UFUNCTION()
    void OnCrosshairColorClicked();

    UFUNCTION()
    void OnCenterDotColorClicked();

    // ---------- 内部逻辑 ----------
    void SwitchSettingsPage(int32 PageIndex);
    void SetShapeParamVisibility(ECrosshairShape Shape);

    FCrosshairConfig BuildConfigFromWidgets() const;
    void ApplyConfigToWidgets(const FCrosshairConfig& Config);
    void ApplyConfigToPreview();

    ECrosshairShape ParseShapeFromString(const FString& Str) const;

    // 颜色状态
    FLinearColor CurrentCrosshairColor = FLinearColor(0.f, 1.f, 0.f, 0.9f);
    FLinearColor CurrentCenterDotColor = FLinearColor::White;
};
