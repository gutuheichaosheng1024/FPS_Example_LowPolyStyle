#include "UI/FPSTitleScreen.h"
#include "UI/FPSSaveGame.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Kismet/KismetSystemLibrary.h"

// 初始化标题界面，绑定按钮点击事件并加载保存的用户名
// 流程：调用 Super::NativeConstruct → 绑定 Start/Settings/Exit 按钮 OnClicked → 从 UFPSSaveGame 加载 PlayerName 并设置输入框 → 绑定 PlayerNameInput 提交事件
void UFPSTitleScreen::NativeConstruct()
{
    Super::NativeConstruct();

    if (StartButton)
        StartButton->OnClicked.AddDynamic(this, &UFPSTitleScreen::OnStartClicked);
    if (SettingsButton)
        SettingsButton->OnClicked.AddDynamic(this, &UFPSTitleScreen::OnSettingsClicked);
    if (ExitButton)
        ExitButton->OnClicked.AddDynamic(this, &UFPSTitleScreen::OnExitClicked);

    if (PlayerNameInput)
    {
        const FString SavedName = UFPSSaveGame::LoadPlayerName();
        PlayerNameInput->SetText(FText::FromString(SavedName));
        PlayerNameInput->OnTextCommitted.AddDynamic(this, &UFPSTitleScreen::OnPlayerNameCommitted);
    }
}

// 开始游戏按钮回调，打开大厅界面
// 流程：通过基类 OpenSubScreen 打开 LobbyScreenClass 子窗口
void UFPSTitleScreen::OnStartClicked()
{
    OpenSubScreen(LobbyScreenClass);
}

// 设置按钮回调，打开设置界面
// 流程：通过基类 OpenSubScreen 打开 SettingsScreenClass 子窗口
void UFPSTitleScreen::OnSettingsClicked()
{
    OpenSubScreen(SettingsScreenClass);
}

// 退出按钮回调，退出游戏
// 流程：调用 UKismetSystemLibrary::QuitGame 退出应用程序
void UFPSTitleScreen::OnExitClicked()
{
    UKismetSystemLibrary::QuitGame(this, nullptr, EQuitPreference::Quit, false);
}

// 玩家名称输入框提交回调，保存非空的玩家名称
// 流程：获取文本并 Trim → 非空则调用 UFPSSaveGame::SavePlayerName 保存
void UFPSTitleScreen::OnPlayerNameCommitted(const FText& Text, ETextCommit::Type CommitType)
{
    FString Name = Text.ToString().TrimStartAndEnd();
    if (!Name.IsEmpty())
    {
        UFPSSaveGame::SavePlayerName(Name);
    }
}
