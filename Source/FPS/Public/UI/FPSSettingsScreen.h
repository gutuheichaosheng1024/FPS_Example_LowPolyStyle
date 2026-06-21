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
 * UFPSSettingsScreen — 设置界面，提供音量、分辨率、全屏和准星的配置与实时预览
 *
 * 职责：管理通用设置（音量/分辨率/全屏）和准星设置（形状/参数/颜色）两个页面；通过 UFPSSaveGame 持久化保存所有配置；提供准星实时预览
 * 使用：UFPSBaseMenuWidget、UFPSSaveGame、UFPSCrosshairWidget、UGameUserSettings、USlider、UComboBoxString、UCheckBox、UButton
 */
UCLASS(Abstract)
class FPS_API UFPSSettingsScreen : public UFPSBaseMenuWidget
{
    GENERATED_BODY()

protected:
    UPROPERTY(meta = (BindWidget))
    USlider* MasterVolumeSlider;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* MasterVolumeText;

    UPROPERTY(meta = (BindWidget))
    USlider* BackgroundVolumeSlider;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* BackgroundVolumeText;

    UPROPERTY(meta = (BindWidget))
    UComboBoxString* ResolutionComboBox;

    UPROPERTY(meta = (BindWidget))
    UCheckBox* FullscreenCheckBox;

    UPROPERTY(meta = (BindWidget))
    UButton* BackButton;

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

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UComboBoxString> CrosshairShapeCombo;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UCheckBox> ShowCenterDotCheckBox;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<USlider> CenterDotRadiusSlider;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<USlider> BarLengthSlider;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<USlider> BarThicknessSlider;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<USlider> BarGapSlider;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<USlider> CircleRadiusSlider;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<USlider> CircleThicknessSlider;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<USlider> CrosshairScaleSlider;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> CrosshairColorButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> CenterDotColorButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UWidget> FourCornerParams;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UWidget> CircleParams;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UFPSCrosshairWidget> PreviewCrosshair;

    virtual void NativeConstruct() override;

private:
    void LoadAndApplySettings();
    TArray<FString> GetSupportedResolutions();
    void ApplyResolution(const FString& Res, bool bFullscreen);

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

    void SwitchSettingsPage(int32 PageIndex);
    void SetShapeParamVisibility(ECrosshairShape Shape);

    FCrosshairConfig BuildConfigFromWidgets() const;
    void ApplyConfigToWidgets(const FCrosshairConfig& Config);
    void ApplyConfigToPreview();

    ECrosshairShape ParseShapeFromString(const FString& Str) const;

    FLinearColor CurrentCrosshairColor = FLinearColor(0.f, 1.f, 0.f, 0.9f);
    FLinearColor CurrentCenterDotColor = FLinearColor::White;
};
