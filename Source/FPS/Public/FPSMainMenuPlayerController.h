#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "FPSMainMenuPlayerController.generated.h"

/**
 * AFPSMainMenuPlayerController — 主菜单专用 PlayerController，仅在 Lvl_MainMenu 中使用
 *
 * 职责：创建标题界面 Widget 并设置 UI 输入模式、应用保存的显示设置（分辨率/全屏）、
 *       离开时重置输入模式为 GameOnly
 * 使用：UFPSSaveGame, UGameUserSettings
 */
UCLASS()
class FPS_API AFPSMainMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UUserWidget> TitleScreenClass;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	UUserWidget* TitleScreen = nullptr;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void ApplySavedDisplaySettings();
};
