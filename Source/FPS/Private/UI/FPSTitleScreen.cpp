#include "UI/FPSTitleScreen.h"
#include "UI/FPSSaveGame.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
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

    // 加载保存的用户名
    if (PlayerNameInput)
    {
        const FString SavedName = UFPSSaveGame::LoadPlayerName();
        PlayerNameInput->SetText(FText::FromString(SavedName));
        PlayerNameInput->OnTextCommitted.AddDynamic(this, &UFPSTitleScreen::OnPlayerNameCommitted);
    }
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

void UFPSTitleScreen::OnPlayerNameCommitted(const FText& Text, ETextCommit::Type CommitType)
{
    FString Name = Text.ToString().TrimStartAndEnd();
    if (!Name.IsEmpty())
    {
        UFPSSaveGame::SavePlayerName(Name);
    }
}
