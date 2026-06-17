#include "UI/FPSSettingsScreen.h"
#include "UI/FPSSaveGame.h"
#include "UI/FPSCrosshairWidget.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/ComboBoxString.h"
#include "Components/CheckBox.h"
#include "Components/Button.h"
#include "Components/Overlay.h"
#include "Components/Widget.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"

void UFPSSettingsScreen::NativeConstruct()
{
    Super::NativeConstruct();

    // ---- 缓存页面引用（Overlay 子控件） ----
    if (SettingsPageContainer)
    {
        if (SettingsPageContainer->GetChildrenCount() >= 1)
            GeneralPage = SettingsPageContainer->GetChildAt(0);
        if (SettingsPageContainer->GetChildrenCount() >= 2)
            CrosshairPage = SettingsPageContainer->GetChildAt(1);
    }

    // ---- 绑定导航按钮 ----
    if (GeneralSettingsButton)
        GeneralSettingsButton->OnClicked.AddDynamic(this, &UFPSSettingsScreen::OnGeneralSettingsClicked);
    if (CrosshairSettingsButton)
        CrosshairSettingsButton->OnClicked.AddDynamic(this, &UFPSSettingsScreen::OnCrosshairSettingsClicked);

    // ---- 绑定通用设置控件 ----
    if (MasterVolumeSlider)
        MasterVolumeSlider->OnValueChanged.AddDynamic(this, &UFPSSettingsScreen::OnMasterVolumeChanged);
    if (BackgroundVolumeSlider)
        BackgroundVolumeSlider->OnValueChanged.AddDynamic(this, &UFPSSettingsScreen::OnBackgroundVolumeChanged);
    if (FullscreenCheckBox)
        FullscreenCheckBox->OnCheckStateChanged.AddDynamic(this, &UFPSSettingsScreen::OnFullscreenChanged);
    if (BackButton)
        BackButton->OnClicked.AddDynamic(this, &UFPSSettingsScreen::OnBackClicked);

    // 填充分辨率下拉框
    if (ResolutionComboBox)
    {
        ResolutionComboBox->ClearOptions();
        for (const FString& Res : GetSupportedResolutions())
        {
            ResolutionComboBox->AddOption(Res);
        }
        ResolutionComboBox->OnSelectionChanged.AddDynamic(this, &UFPSSettingsScreen::OnResolutionChanged);
    }

    // ---- 绑定准星控件 ----
    if (CrosshairShapeCombo)
    {
        CrosshairShapeCombo->ClearOptions();
        CrosshairShapeCombo->AddOption(TEXT("四角"));
        CrosshairShapeCombo->AddOption(TEXT("圆形"));
        CrosshairShapeCombo->AddOption(TEXT("无"));
        CrosshairShapeCombo->OnSelectionChanged.AddDynamic(this, &UFPSSettingsScreen::OnCrosshairShapeChanged);
    }

    // 准星 Slider 全部绑定同一个刷新回调
    auto BindCrosshairSlider = [this](USlider* Slider)
    {
        if (Slider)
            Slider->OnValueChanged.AddDynamic(this, &UFPSSettingsScreen::OnAnyCrosshairParamChanged);
    };
    BindCrosshairSlider(CenterDotRadiusSlider);
    BindCrosshairSlider(BarLengthSlider);
    BindCrosshairSlider(BarThicknessSlider);
    BindCrosshairSlider(BarGapSlider);
    BindCrosshairSlider(CircleRadiusSlider);
    BindCrosshairSlider(CircleThicknessSlider);
    BindCrosshairSlider(CrosshairScaleSlider);

    if (ShowCenterDotCheckBox)
        ShowCenterDotCheckBox->OnCheckStateChanged.AddDynamic(this, &UFPSSettingsScreen::OnCrosshairCheckChanged);

    // 颜色 Button
    if (CrosshairColorButton)
        CrosshairColorButton->OnClicked.AddDynamic(this, &UFPSSettingsScreen::OnCrosshairColorClicked);
    if (CenterDotColorButton)
        CenterDotColorButton->OnClicked.AddDynamic(this, &UFPSSettingsScreen::OnCenterDotColorClicked);

    // ---- 加载设置 ----
    LoadAndApplySettings();

    // 默认显示通用设置页
    SwitchSettingsPage(0);
}

void UFPSSettingsScreen::LoadAndApplySettings()
{
    UFPSSaveGame* Settings = UFPSSaveGame::LoadSettings();
    if (!Settings) return;

    if (MasterVolumeSlider)
    {
        MasterVolumeSlider->SetValue(Settings->MasterVolume);
        if (MasterVolumeText)
            MasterVolumeText->SetText(FText::AsPercent(Settings->MasterVolume));
    }

    if (BackgroundVolumeSlider)
    {
        BackgroundVolumeSlider->SetValue(Settings->BackgroundVolume);
        if (BackgroundVolumeText)
            BackgroundVolumeText->SetText(FText::AsPercent(Settings->BackgroundVolume));
    }

    if (ResolutionComboBox)
    {
        ResolutionComboBox->SetSelectedOption(Settings->Resolution);
    }

    if (FullscreenCheckBox)
    {
        FullscreenCheckBox->SetIsChecked(Settings->bFullscreen);
    }

    // 立即应用分辨率和全屏设置
    ApplyResolution(Settings->Resolution, Settings->bFullscreen);

    // 加载准星配置
    const FCrosshairConfig CrosshairSaved = UFPSSaveGame::LoadCrosshairConfig();
    ApplyConfigToWidgets(CrosshairSaved);
    ApplyConfigToPreview();
}

TArray<FString> UFPSSettingsScreen::GetSupportedResolutions()
{
    TArray<FString> Resolutions;
    TArray<FIntPoint> SupportedRes;
    UKismetSystemLibrary::GetSupportedFullscreenResolutions(SupportedRes);

    // 按宽度降序排列
    SupportedRes.Sort([](const FIntPoint& A, const FIntPoint& B) {
        return A.X > B.X || (A.X == B.X && A.Y > B.Y);
    });

    for (const FIntPoint& Res : SupportedRes)
    {
        Resolutions.Add(FString::Printf(TEXT("%dx%d"), Res.X, Res.Y));
    }

    return Resolutions;
}

void UFPSSettingsScreen::ApplyResolution(const FString& Res, bool bFullscreen)
{
    int32 Width = 0, Height = 0;
    FString Left, Right;
    if (Res.Split(TEXT("x"), &Left, &Right))
    {
        Width = FCString::Atoi(*Left);
        Height = FCString::Atoi(*Right);
    }

    UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
    if (Settings && Width > 0 && Height > 0)
    {
        const FIntPoint DesiredResolution(Width, Height);

        // 设置分辨率
        Settings->SetScreenResolution(DesiredResolution);

        // 设置窗口模式
        if (bFullscreen)
        {
            Settings->SetFullscreenMode(EWindowMode::Fullscreen);
        }
        else
        {
            Settings->SetFullscreenMode(EWindowMode::Windowed);
        }

        // 应用设置
        Settings->ApplySettings(false);

        // 全屏模式下确认分辨率
        if (bFullscreen)
        {
            Settings->SetScreenResolution(DesiredResolution);
            Settings->ApplySettings(false);
        }
    }
}

void UFPSSettingsScreen::OnMasterVolumeChanged(float Value)
{
    if (MasterVolumeText)
        MasterVolumeText->SetText(FText::AsPercent(Value));
}

void UFPSSettingsScreen::OnBackgroundVolumeChanged(float Value)
{
    if (BackgroundVolumeText)
        BackgroundVolumeText->SetText(FText::AsPercent(Value));
}

void UFPSSettingsScreen::OnResolutionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    // 仅处理用户交互（排除代码触发的）
    if (SelectionType == ESelectInfo::OnMouseClick || SelectionType == ESelectInfo::OnKeyPress)
    {
        bool bFullscreen = FullscreenCheckBox ? FullscreenCheckBox->IsChecked() : false;
        ApplyResolution(SelectedItem, bFullscreen);
    }
}

void UFPSSettingsScreen::OnFullscreenChanged(bool bIsChecked)
{
    FString Res = ResolutionComboBox ? ResolutionComboBox->GetSelectedOption() : TEXT("1920x1080");
    ApplyResolution(Res, bIsChecked);
}

void UFPSSettingsScreen::OnBackClicked()
{
    // 保存通用设置
    UFPSSaveGame::SaveSettings(
        MasterVolumeSlider ? MasterVolumeSlider->GetValue() : 1.0f,
        BackgroundVolumeSlider ? BackgroundVolumeSlider->GetValue() : 0.5f,
        ResolutionComboBox ? ResolutionComboBox->GetSelectedOption() : TEXT("1920x1080"),
        FullscreenCheckBox ? FullscreenCheckBox->IsChecked() : true
    );

    // 保存准星配置
    UFPSSaveGame::SaveCrosshairConfig(BuildConfigFromWidgets());

    CloseSelf();
}

// ======================================================================
// 页面导航
// ======================================================================

void UFPSSettingsScreen::OnGeneralSettingsClicked()
{
    SwitchSettingsPage(0);
}

void UFPSSettingsScreen::OnCrosshairSettingsClicked()
{
    SwitchSettingsPage(1);
}

void UFPSSettingsScreen::SwitchSettingsPage(int32 PageIndex)
{
    const ESlateVisibility Vis0 = (PageIndex == 0) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
    const ESlateVisibility Vis1 = (PageIndex == 1) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;

    if (GeneralPage)
        GeneralPage->SetVisibility(Vis0);
    if (CrosshairPage)
        CrosshairPage->SetVisibility(Vis1);
}

// ======================================================================
// 准星参数变更
// ======================================================================

void UFPSSettingsScreen::OnAnyCrosshairParamChanged(float Value)
{
    ApplyConfigToPreview();
}

void UFPSSettingsScreen::OnCrosshairCheckChanged(bool bChecked)
{
    ApplyConfigToPreview();
}

void UFPSSettingsScreen::OnCrosshairShapeChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    const ECrosshairShape Shape = ParseShapeFromString(SelectedItem);
    SetShapeParamVisibility(Shape);
    ApplyConfigToPreview();
}

void UFPSSettingsScreen::SetShapeParamVisibility(ECrosshairShape Shape)
{
    if (FourCornerParams)
        FourCornerParams->SetVisibility(
            Shape == ECrosshairShape::FourCorner ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    if (CircleParams)
        CircleParams->SetVisibility(
            Shape == ECrosshairShape::Circle ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

// ======================================================================
// 颜色选择（简化实现：颜色值固定，不弹出颜色选择器）
// 运行时使用简化方案，蓝图子类可覆盖此行为
// ======================================================================

void UFPSSettingsScreen::OnCrosshairColorClicked()
{
    // 简化实现：循环切换预设颜色
    static const TArray<FLinearColor> PresetColors = {
        FLinearColor(0.0f, 1.0f, 0.0f, 0.9f),  // 绿色（默认）
        FLinearColor(1.0f, 0.0f, 0.0f, 0.9f),  // 红色
        FLinearColor(0.0f, 0.0f, 1.0f, 0.9f),  // 蓝色
        FLinearColor(1.0f, 1.0f, 0.0f, 0.9f),  // 黄色
        FLinearColor(1.0f, 1.0f, 1.0f, 0.9f),  // 白色
    };

    static int32 ColorIndex = 0;
    ColorIndex = (ColorIndex + 1) % PresetColors.Num();
    CurrentCrosshairColor = PresetColors[ColorIndex];
    ApplyConfigToPreview();
}

void UFPSSettingsScreen::OnCenterDotColorClicked()
{
    // 简化实现：循环切换预设颜色
    static const TArray<FLinearColor> PresetColors = {
        FLinearColor::White,
        FLinearColor::Red,
        FLinearColor::Green,
        FLinearColor::Yellow,
    };

    static int32 ColorIndex = 0;
    ColorIndex = (ColorIndex + 1) % PresetColors.Num();
    CurrentCenterDotColor = PresetColors[ColorIndex];
    ApplyConfigToPreview();
}

// ======================================================================
// Config ↔ Widgets 双向转换
// ======================================================================

FCrosshairConfig UFPSSettingsScreen::BuildConfigFromWidgets() const
{
    FCrosshairConfig Config;

    // 形状
    if (CrosshairShapeCombo)
    {
        Config.Shape = ParseShapeFromString(CrosshairShapeCombo->GetSelectedOption());
    }

    // 中心点
    Config.bShowCenterDot = ShowCenterDotCheckBox ? ShowCenterDotCheckBox->IsChecked() : true;
    Config.CenterDotRadius = CenterDotRadiusSlider ? CenterDotRadiusSlider->GetValue() : 2.f;
    Config.CenterDotColor = CurrentCenterDotColor;

    // 四角
    Config.BarLength = BarLengthSlider ? BarLengthSlider->GetValue() : 14.f;
    Config.BarThickness = BarThicknessSlider ? BarThicknessSlider->GetValue() : 3.f;
    Config.BarGap = BarGapSlider ? BarGapSlider->GetValue() : 6.f;

    // 圆形
    Config.CircleRadius = CircleRadiusSlider ? CircleRadiusSlider->GetValue() : 14.f;
    Config.CircleThickness = CircleThicknessSlider ? CircleThicknessSlider->GetValue() : 2.f;

    // 颜色 + 缩放
    Config.CrosshairColor = CurrentCrosshairColor;
    Config.OverallScale = CrosshairScaleSlider ? CrosshairScaleSlider->GetValue() : 1.f;

    return Config;
}

void UFPSSettingsScreen::ApplyConfigToWidgets(const FCrosshairConfig& Config)
{
    if (CrosshairShapeCombo)
    {
        switch (Config.Shape)
        {
        case ECrosshairShape::Circle:
            CrosshairShapeCombo->SetSelectedOption(TEXT("圆形")); break;
        case ECrosshairShape::None:
            CrosshairShapeCombo->SetSelectedOption(TEXT("无")); break;
        default:
            CrosshairShapeCombo->SetSelectedOption(TEXT("四角")); break;
        }
    }

    if (ShowCenterDotCheckBox) ShowCenterDotCheckBox->SetIsChecked(Config.bShowCenterDot);
    if (CenterDotRadiusSlider) CenterDotRadiusSlider->SetValue(Config.CenterDotRadius);
    if (BarLengthSlider) BarLengthSlider->SetValue(Config.BarLength);
    if (BarThicknessSlider) BarThicknessSlider->SetValue(Config.BarThickness);
    if (BarGapSlider) BarGapSlider->SetValue(Config.BarGap);
    if (CircleRadiusSlider) CircleRadiusSlider->SetValue(Config.CircleRadius);
    if (CircleThicknessSlider) CircleThicknessSlider->SetValue(Config.CircleThickness);
    if (CrosshairScaleSlider) CrosshairScaleSlider->SetValue(Config.OverallScale);

    CurrentCrosshairColor = Config.CrosshairColor;
    CurrentCenterDotColor = Config.CenterDotColor;

    SetShapeParamVisibility(Config.Shape);
}

void UFPSSettingsScreen::ApplyConfigToPreview()
{
    if (PreviewCrosshair)
    {
        PreviewCrosshair->ApplyConfig(BuildConfigFromWidgets());
    }
}

// ======================================================================
// 工具
// ======================================================================

ECrosshairShape UFPSSettingsScreen::ParseShapeFromString(const FString& Str) const
{
    if (Str == TEXT("圆形"))
        return ECrosshairShape::Circle;
    if (Str == TEXT("无"))
        return ECrosshairShape::None;
    return ECrosshairShape::FourCorner;
}
