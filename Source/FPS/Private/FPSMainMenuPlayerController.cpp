#include "FPSMainMenuPlayerController.h"
#include "Blueprint/UserWidget.h"
#include "UI/FPSSaveGame.h"
#include "GameFramework/GameUserSettings.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"

void AFPSMainMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Log, TEXT("MainMenuPC: BeginPlay, IsLocal=%d, TitleScreenClass=%s"),
		IsLocalController(),
		TitleScreenClass ? *TitleScreenClass->GetName() : TEXT("NULL"));

	if (!IsLocalController()) return;

	// 应用保存的显示设置
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

void AFPSMainMenuPlayerController::ApplySavedDisplaySettings()
{
	UFPSSaveGame* Settings = UFPSSaveGame::LoadSettings();
	if (!Settings) return;

	UGameUserSettings* GameSettings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	if (!GameSettings) return;

	// 解析分辨率
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

		// 设置分辨率
		GameSettings->SetScreenResolution(DesiredResolution);

		// 设置窗口模式
		if (Settings->bFullscreen)
		{
			GameSettings->SetFullscreenMode(EWindowMode::Fullscreen);
		}
		else
		{
			GameSettings->SetFullscreenMode(EWindowMode::Windowed);
		}

		// 应用设置
		GameSettings->ApplySettings(false);

		UE_LOG(LogTemp, Log, TEXT("MainMenuPC: Applied display settings - %dx%d, Fullscreen=%d"),
			Width, Height, Settings->bFullscreen);
	}
}

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
