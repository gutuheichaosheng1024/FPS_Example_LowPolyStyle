#include "FPSMainMenuPlayerController.h"
#include "Blueprint/UserWidget.h"
#include "UI/FPSSaveGame.h"
#include "GameFramework/GameUserSettings.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"

// BeginPlay：初始化主菜单 UI，应用保存的显示设置并创建标题界面
// 流程：调用 Super → 检查 IsLocalController → ApplySavedDisplaySettings → 创建 TitleScreen 并 AddToViewport → 设置 UIOnly 输入模式
void AFPSMainMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Log, TEXT("MainMenuPC: BeginPlay, IsLocal=%d, TitleScreenClass=%s"),
		IsLocalController(),
		TitleScreenClass ? *TitleScreenClass->GetName() : TEXT("NULL"));

	if (!IsLocalController()) return;

	ApplySavedDisplaySettings();

	if (TitleScreenClass)
	{
		TitleScreen = CreateWidget<UUserWidget>(this, TitleScreenClass);
		if (TitleScreen)
		{
			TitleScreen->AddToViewport(0);
			UE_LOG(LogTemp, Log, TEXT("MainMenuPC: TitleScreen created and added to viewport"));

			FInputModeUIOnly InputMode;
			InputMode.SetWidgetToFocus(TitleScreen->TakeWidget());
			SetInputMode(InputMode);
			SetShowMouseCursor(true);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("MainMenuPC: CreateWidget returned NULL"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("MainMenuPC: TitleScreenClass not set!"));
	}
}

// ApplySavedDisplaySettings：从存档读取分辨率/全屏设置并应用到 GameUserSettings
// 流程：LoadSettings → 获取 GameUserSettings → 解析 Resolution 字符串 → SetScreenResolution → SetFullscreenMode → ApplySettings
void AFPSMainMenuPlayerController::ApplySavedDisplaySettings()
{
	UFPSSaveGame* Settings = UFPSSaveGame::LoadSettings();
	if (!Settings) return;

	UGameUserSettings* GameSettings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	if (!GameSettings) return;

	int32 Width = 0, Height = 0;
	FString Left, Right;
	if (Settings->Resolution.Split(TEXT("x"), &Left, &Right))
	{
		Width = FCString::Atoi(*Left);
		Height = FCString::Atoi(*Right);
	}

	if (Width > 0 && Height > 0)
	{
		const FIntPoint DesiredResolution(Width, Height);

		GameSettings->SetScreenResolution(DesiredResolution);

		if (Settings->bFullscreen)
		{
			GameSettings->SetFullscreenMode(EWindowMode::Fullscreen);
		}
		else
		{
			GameSettings->SetFullscreenMode(EWindowMode::Windowed);
		}

		GameSettings->ApplySettings(false);

		UE_LOG(LogTemp, Log, TEXT("MainMenuPC: Applied display settings - %dx%d, Fullscreen=%d"),
			Width, Height, Settings->bFullscreen);
	}
}

// EndPlay：离开主菜单时重置输入模式为 GameOnly
// 流程：检查 IsLocalController → 设置 GameOnly 输入模式 → 隐藏鼠标 → 调用 Super::EndPlay
void AFPSMainMenuPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsLocalController())
	{
		FInputModeGameOnly GameMode;
		SetInputMode(GameMode);
		SetShowMouseCursor(false);

		UE_LOG(LogTemp, Log, TEXT("MainMenuPC: EndPlay, input mode reset to GameOnly"));
	}

	Super::EndPlay(EndPlayReason);
}
