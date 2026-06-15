#pragma once

#include "CoreMinimal.h"
#include "UI/FPSBaseMenuWidget.h"
#include "FPSTitleScreen.generated.h"

class UButton;

/**
 * 标题界面
 * 提供开始游戏、设置、退出三个入口
 */
UCLASS(Abstract)
class FPS_API UFPSTitleScreen : public UFPSBaseMenuWidget
{
    GENERATED_BODY()

protected:
    /** 开始游戏按钮 */
    UPROPERTY(meta = (BindWidget))
    UButton* StartButton;

    /** 设置按钮 */
    UPROPERTY(meta = (BindWidget))
    UButton* SettingsButton;

    /** 退出按钮 */
    UPROPERTY(meta = (BindWidget))
    UButton* ExitButton;

    /** 大厅界面类（蓝图 ClassDefaults 中设置） */
    UPROPERTY(EditDefaultsOnly, Category = "Navigation")
    TSubclassOf<UUserWidget> LobbyScreenClass;

    /** 设置界面类（蓝图 ClassDefaults 中设置） */
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
};
