#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "FPSMainMenuPlayerController.generated.h"

/**
 * 主菜单专用 PlayerController
 * 仅在主菜单地图(Lvl_MainMenu)中使用
 * 负责创建主菜单 Widget 并设置 UI 输入模式
 *
 * 游戏地图(Lvl_Shooter)继续使用原有的 PlayerController，不受影响
 */
UCLASS()
class FPS_API AFPSMainMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	/** 标题界面类（蓝图子类中设置） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UUserWidget> TitleScreenClass;

	/** 当前标题界面实例 */
	UPROPERTY(BlueprintReadOnly, Category = "UI")
	UUserWidget* TitleScreen = nullptr;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** 应用保存的显示设置（分辨率、全屏模式） */
	void ApplySavedDisplaySettings();
};
