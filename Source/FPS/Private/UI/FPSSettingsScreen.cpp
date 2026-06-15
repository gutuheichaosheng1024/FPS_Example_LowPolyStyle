#include "UI/FPSSettingsScreen.h"
#include "UI/FPSSaveGame.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/ComboBoxString.h"
#include "Components/CheckBox.h"
#include "Components/Button.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"

void UFPSSettingsScreen::NativeConstruct()
{
    Super::NativeConstruct();

    // 绑定滑动条事件
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
        // 绑定下拉框选择事件（选择后立即生效）
        ResolutionComboBox->OnSelectionChanged.AddDynamic(this, &UFPSSettingsScreen::OnResolutionChanged);
    }

    LoadAndApplySettings();
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
    // 保存设置（分辨率和全屏已在选择时立即生效，此处只需保存）
    UFPSSaveGame::SaveSettings(
        MasterVolumeSlider ? MasterVolumeSlider->GetValue() : 1.0f,
        BackgroundVolumeSlider ? BackgroundVolumeSlider->GetValue() : 0.5f,
        ResolutionComboBox ? ResolutionComboBox->GetSelectedOption() : TEXT("1920x1080"),
        FullscreenCheckBox ? FullscreenCheckBox->IsChecked() : true
    );

    CloseSelf();
}
