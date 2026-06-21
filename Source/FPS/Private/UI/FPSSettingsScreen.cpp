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

// 初始化设置界面，缓存页面引用、绑定所有控件事件、加载并应用保存的设置
// 流程：调用 Super::NativeConstruct → 从 Overlay 缓存 GeneralPage 和 CrosshairPage → 绑定导航按钮 → 绑定通用设置控件 → 填充分辨率下拉框 → 绑定准星控件和 Slider → 绑定颜色按钮 → 调用 LoadAndApplySettings → 默认显示通用设置页
void UFPSSettingsScreen::NativeConstruct()
{
    Super::NativeConstruct();

    if (SettingsPageContainer)
    {
        if (SettingsPageContainer->GetChildrenCount() >= 1)
            GeneralPage = SettingsPageContainer->GetChildAt(0);
        if (SettingsPageContainer->GetChildrenCount() >= 2)
            CrosshairPage = SettingsPageContainer->GetChildAt(1);
    }

    if (GeneralSettingsButton)
        GeneralSettingsButton->OnClicked.AddDynamic(this, &UFPSSettingsScreen::OnGeneralSettingsClicked);
    if (CrosshairSettingsButton)
        CrosshairSettingsButton->OnClicked.AddDynamic(this, &UFPSSettingsScreen::OnCrosshairSettingsClicked);

    if (MasterVolumeSlider)
        MasterVolumeSlider->OnValueChanged.AddDynamic(this, &UFPSSettingsScreen::OnMasterVolumeChanged);
    if (BackgroundVolumeSlider)
        BackgroundVolumeSlider->OnValueChanged.AddDynamic(this, &UFPSSettingsScreen::OnBackgroundVolumeChanged);
    if (FullscreenCheckBox)
        FullscreenCheckBox->OnCheckStateChanged.AddDynamic(this, &UFPSSettingsScreen::OnFullscreenChanged);
    if (BackButton)
        BackButton->OnClicked.AddDynamic(this, &UFPSSettingsScreen::OnBackClicked);

    if (ResolutionComboBox)
    {
        ResolutionComboBox->ClearOptions();
        for (const FString& Res : GetSupportedResolutions())
        {
            ResolutionComboBox->AddOption(Res);
        }
        ResolutionComboBox->OnSelectionChanged.AddDynamic(this, &UFPSSettingsScreen::OnResolutionChanged);
    }

    if (CrosshairShapeCombo)
    {
        CrosshairShapeCombo->ClearOptions();
        CrosshairShapeCombo->AddOption(TEXT("四角"));
        CrosshairShapeCombo->AddOption(TEXT("圆形"));
        CrosshairShapeCombo->AddOption(TEXT("无"));
        CrosshairShapeCombo->OnSelectionChanged.AddDynamic(this, &UFPSSettingsScreen::OnCrosshairShapeChanged);
    }

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

    if (CrosshairColorButton)
        CrosshairColorButton->OnClicked.AddDynamic(this, &UFPSSettingsScreen::OnCrosshairColorClicked);
    if (CenterDotColorButton)
        CenterDotColorButton->OnClicked.AddDynamic(this, &UFPSSettingsScreen::OnCenterDotColorClicked);

    LoadAndApplySettings();

    SwitchSettingsPage(0);
}

// 加载保存的设置并应用到所有控件
// 流程：从 UFPSSaveGame 加载设置对象 → 逐个设置 Slider/ComboBox/CheckBox 的值 → 调用 ApplyResolution 应用分辨率和全屏 → 加载准星配置到控件和预览
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

    ApplyResolution(Settings->Resolution, Settings->bFullscreen);

    const FCrosshairConfig CrosshairSaved = UFPSSaveGame::LoadCrosshairConfig();
    ApplyConfigToWidgets(CrosshairSaved);
    ApplyConfigToPreview();
}

// 获取系统支持的分辨率列表，按宽度降序排列
// 流程：调用 UKismetSystemLibrary::GetSupportedFullscreenResolutions → 按 X 降序排序 → 格式化为 "WxH" 字符串并返回
TArray<FString> UFPSSettingsScreen::GetSupportedResolutions()
{
    TArray<FString> Resolutions;
    TArray<FIntPoint> SupportedRes;
    UKismetSystemLibrary::GetSupportedFullscreenResolutions(SupportedRes);

    SupportedRes.Sort([](const FIntPoint& A, const FIntPoint& B) {
        return A.X > B.X || (A.X == B.X && A.Y > B.Y);
    });

    for (const FIntPoint& Res : SupportedRes)
    {
        Resolutions.Add(FString::Printf(TEXT("%dx%d"), Res.X, Res.Y));
    }

    return Resolutions;
}

// 应用分辨率和全屏/窗口模式设置
// 流程：解析 "WxH" 字符串 → 通过 GameUserSettings 设置分辨率和窗口模式 → 调用 ApplySettings 生效 → 全屏模式下再次确认分辨率
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

        Settings->SetScreenResolution(DesiredResolution);

        if (bFullscreen)
        {
            Settings->SetFullscreenMode(EWindowMode::Fullscreen);
        }
        else
        {
            Settings->SetFullscreenMode(EWindowMode::Windowed);
        }

        Settings->ApplySettings(false);

        if (bFullscreen)
        {
            Settings->SetScreenResolution(DesiredResolution);
            Settings->ApplySettings(false);
        }
    }
}

// 主音量滑块变更回调，更新百分比文本显示
// 流程：更新 MasterVolumeText 为百分比格式
void UFPSSettingsScreen::OnMasterVolumeChanged(float Value)
{
    if (MasterVolumeText)
        MasterVolumeText->SetText(FText::AsPercent(Value));
}

// 背景音量滑块变更回调，更新百分比文本显示
// 流程：更新 BackgroundVolumeText 为百分比格式
void UFPSSettingsScreen::OnBackgroundVolumeChanged(float Value)
{
    if (BackgroundVolumeText)
        BackgroundVolumeText->SetText(FText::AsPercent(Value));
}

// 分辨率下拉框变更回调，仅在用户交互时应用分辨率
// 流程：检查 SelectionType 是否为鼠标或键盘触发 → 获取当前全屏状态 → 调用 ApplyResolution
void UFPSSettingsScreen::OnResolutionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    if (SelectionType == ESelectInfo::OnMouseClick || SelectionType == ESelectInfo::OnKeyPress)
    {
        bool bFullscreen = FullscreenCheckBox ? FullscreenCheckBox->IsChecked() : false;
        ApplyResolution(SelectedItem, bFullscreen);
    }
}

// 全屏复选框变更回调，应用当前分辨率和全屏状态
// 流程：获取 ComboBox 当前选中分辨率 → 调用 ApplyResolution
void UFPSSettingsScreen::OnFullscreenChanged(bool bIsChecked)
{
    FString Res = ResolutionComboBox ? ResolutionComboBox->GetSelectedOption() : TEXT("1920x1080");
    ApplyResolution(Res, bIsChecked);
}

// 返回按钮回调，保存通用设置和准星配置后关闭自身
// 流程：读取所有控件当前值 → 调用 UFPSSaveGame::SaveSettings 保存通用设置 → 调用 SaveCrosshairConfig 保存准星配置 → 调用 CloseSelf
void UFPSSettingsScreen::OnBackClicked()
{
    UFPSSaveGame::SaveSettings(
        MasterVolumeSlider ? MasterVolumeSlider->GetValue() : 1.0f,
        BackgroundVolumeSlider ? BackgroundVolumeSlider->GetValue() : 0.5f,
        ResolutionComboBox ? ResolutionComboBox->GetSelectedOption() : TEXT("1920x1080"),
        FullscreenCheckBox ? FullscreenCheckBox->IsChecked() : true
    );

    UFPSSaveGame::SaveCrosshairConfig(BuildConfigFromWidgets());

    CloseSelf();
}

// 切换到通用设置页（页面索引 0）
// 流程：调用 SwitchSettingsPage(0)
void UFPSSettingsScreen::OnGeneralSettingsClicked()
{
    SwitchSettingsPage(0);
}

// 切换到准星设置页（页面索引 1）
// 流程：调用 SwitchSettingsPage(1)
void UFPSSettingsScreen::OnCrosshairSettingsClicked()
{
    SwitchSettingsPage(1);
}

// 切换设置页面，通过 Overlay 子控件的可见性控制显示哪个页面
// 流程：根据 PageIndex 设置 GeneralPage 和 CrosshairPage 的 Visible/Collapsed 状态
void UFPSSettingsScreen::SwitchSettingsPage(int32 PageIndex)
{
    const ESlateVisibility Vis0 = (PageIndex == 0) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
    const ESlateVisibility Vis1 = (PageIndex == 1) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;

    if (GeneralPage)
        GeneralPage->SetVisibility(Vis0);
    if (CrosshairPage)
        CrosshairPage->SetVisibility(Vis1);
}

// 任意准星 Slider 参数变更回调，刷新预览
// 流程：调用 ApplyConfigToPreview 更新预览准星
void UFPSSettingsScreen::OnAnyCrosshairParamChanged(float Value)
{
    ApplyConfigToPreview();
}

// 准星 CheckBox 状态变更回调，刷新预览
// 流程：调用 ApplyConfigToPreview 更新预览准星
void UFPSSettingsScreen::OnCrosshairCheckChanged(bool bChecked)
{
    ApplyConfigToPreview();
}

// 准星形状下拉框变更回调，更新参数区可见性并刷新预览
// 流程：解析选中项为 ECrosshairShape → 调用 SetShapeParamVisibility 切换参数区 → 调用 ApplyConfigToPreview
void UFPSSettingsScreen::OnCrosshairShapeChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    const ECrosshairShape Shape = ParseShapeFromString(SelectedItem);
    SetShapeParamVisibility(Shape);
    ApplyConfigToPreview();
}

// 根据准星形状切换四角参数区和圆形参数区的可见性
// 流程：FourCorner 形状显示 FourCornerParams 否则显示 CircleParams → 圆形形状显示 CircleParams 否则显示 FourCornerParams
void UFPSSettingsScreen::SetShapeParamVisibility(ECrosshairShape Shape)
{
    if (FourCornerParams)
        FourCornerParams->SetVisibility(
            Shape == ECrosshairShape::FourCorner ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    if (CircleParams)
        CircleParams->SetVisibility(
            Shape == ECrosshairShape::Circle ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

// 准星颜色按钮回调，循环切换预设颜色
// 流程：静态 PresetColors 数组中循环索引 → 更新 CurrentCrosshairColor → 刷新预览
void UFPSSettingsScreen::OnCrosshairColorClicked()
{
    static const TArray<FLinearColor> PresetColors = {
        FLinearColor(0.0f, 1.0f, 0.0f, 0.9f),
        FLinearColor(1.0f, 0.0f, 0.0f, 0.9f),
        FLinearColor(0.0f, 0.0f, 1.0f, 0.9f),
        FLinearColor(1.0f, 1.0f, 0.0f, 0.9f),
        FLinearColor(1.0f, 1.0f, 1.0f, 0.9f),
    };

    static int32 ColorIndex = 0;
    ColorIndex = (ColorIndex + 1) % PresetColors.Num();
    CurrentCrosshairColor = PresetColors[ColorIndex];
    ApplyConfigToPreview();
}

// 中心点颜色按钮回调，循环切换预设颜色
// 流程：静态 PresetColors 数组中循环索引 → 更新 CurrentCenterDotColor → 刷新预览
void UFPSSettingsScreen::OnCenterDotColorClicked()
{
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

// 从当前控件值构建完整的准星配置结构体
// 流程：从 ComboBox 解析形状 → 读取 CenterDot/四角/圆形各控件的值 → 读取颜色和缩放 → 组装 FCrosshairConfig 返回
FCrosshairConfig UFPSSettingsScreen::BuildConfigFromWidgets() const
{
    FCrosshairConfig Config;

    if (CrosshairShapeCombo)
    {
        Config.Shape = ParseShapeFromString(CrosshairShapeCombo->GetSelectedOption());
    }

    Config.bShowCenterDot = ShowCenterDotCheckBox ? ShowCenterDotCheckBox->IsChecked() : true;
    Config.CenterDotRadius = CenterDotRadiusSlider ? CenterDotRadiusSlider->GetValue() : 2.f;
    Config.CenterDotColor = CurrentCenterDotColor;

    Config.BarLength = BarLengthSlider ? BarLengthSlider->GetValue() : 14.f;
    Config.BarThickness = BarThicknessSlider ? BarThicknessSlider->GetValue() : 3.f;
    Config.BarGap = BarGapSlider ? BarGapSlider->GetValue() : 6.f;

    Config.CircleRadius = CircleRadiusSlider ? CircleRadiusSlider->GetValue() : 14.f;
    Config.CircleThickness = CircleThicknessSlider ? CircleThicknessSlider->GetValue() : 2.f;

    Config.CrosshairColor = CurrentCrosshairColor;
    Config.OverallScale = CrosshairScaleSlider ? CrosshairScaleSlider->GetValue() : 1.f;

    return Config;
}

// 将准星配置结构体应用回所有控件
// 流程：设置 ComboBox 选中项 → 设置各 Slider 和 CheckBox 的值 → 更新颜色成员变量 → 调用 SetShapeParamVisibility
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

// 将当前控件配置应用到预览准星控件
// 流程：BuildConfigFromWidgets 构建配置 → PreviewCrosshair->ApplyConfig 应用
void UFPSSettingsScreen::ApplyConfigToPreview()
{
    if (PreviewCrosshair)
    {
        PreviewCrosshair->ApplyConfig(BuildConfigFromWidgets());
    }
}

// 将中文字符串解析为 ECrosshairShape 枚举
// 流程：匹配 "圆形" 返回 Circle → 匹配 "无" 返回 None → 默认返回 FourCorner
ECrosshairShape UFPSSettingsScreen::ParseShapeFromString(const FString& Str) const
{
    if (Str == TEXT("圆形"))
        return ECrosshairShape::Circle;
    if (Str == TEXT("无"))
        return ECrosshairShape::None;
    return ECrosshairShape::FourCorner;
}
