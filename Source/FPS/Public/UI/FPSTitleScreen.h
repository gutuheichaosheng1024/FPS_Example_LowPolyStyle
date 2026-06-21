#pragma once

#include "CoreMinimal.h"
#include "UI/FPSBaseMenuWidget.h"
#include "FPSTitleScreen.generated.h"

class UButton;
class UEditableTextBox;

/**
 * UFPSTitleScreen — 标题界面，提供开始游戏、设置和退出三个入口按钮
 *
 * 职责：处理主菜单入口按钮点击事件；加载和保存玩家名称到本地存档；通过基类 OpenSubScreen 导航到大厅或设置界面
 * 使用：UFPSBaseMenuWidget、UFPSSaveGame、UButton、UEditableTextBox
 */
UCLASS(Abstract)
class FPS_API UFPSTitleScreen : public UFPSBaseMenuWidget
{
    GENERATED_BODY()

protected:
    UPROPERTY(meta = (BindWidget))
    UButton* StartButton;

    UPROPERTY(meta = (BindWidget))
    UButton* SettingsButton;

    UPROPERTY(meta = (BindWidget))
    UButton* ExitButton;

    UPROPERTY(meta = (BindWidget))
    UEditableTextBox* PlayerNameInput;

    UPROPERTY(EditDefaultsOnly, Category = "Navigation")
    TSubclassOf<UUserWidget> LobbyScreenClass;

    UPROPERTY(EditDefaultsOnly, Category = "Navigation")
    TSubclassOf<UUserWidget> SettingsScreenClass;

    virtual void NativeConstruct() override;

private:
    UFUNCTION()
    void OnStartClicked();

    UFUNCTION()
    void OnSettingsClicked();

    UFUNCTION()
    void OnExitClicked();

    UFUNCTION()
    void OnPlayerNameCommitted(const FText& Text, ETextCommit::Type CommitType);
};
