#include "UI/FPSTitleScreen.h"
#include "Components/Button.h"
#include "Kismet/KismetSystemLibrary.h"

void UFPSTitleScreen::NativeConstruct()
{
    Super::NativeConstruct();

    if (StartButton)
        StartButton->OnClicked.AddDynamic(this, &UFPSTitleScreen::OnStartClicked);
    if (SettingsButton)
        SettingsButton->OnClicked.AddDynamic(this, &UFPSTitleScreen::OnSettingsClicked);
    if (ExitButton)
        ExitButton->OnClicked.AddDynamic(this, &UFPSTitleScreen::OnExitClicked);
}

void UFPSTitleScreen::OnStartClicked()
{
    OpenSubScreen(LobbyScreenClass);
}

void UFPSTitleScreen::OnSettingsClicked()
{
    OpenSubScreen(SettingsScreenClass);
}

void UFPSTitleScreen::OnExitClicked()
{
    UKismetSystemLibrary::QuitGame(this, nullptr, EQuitPreference::Quit, false);
}
