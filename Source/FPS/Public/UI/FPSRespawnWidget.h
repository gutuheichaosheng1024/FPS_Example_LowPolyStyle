#pragma once

#include "CoreMinimal.h"
#include "UI/FPSBaseMenuWidget.h"
#include "FPSRespawnWidget.generated.h"

class UTextBlock;
class UButton;

/**
 * 重生界面
 * 显示击杀者名称，提供重生按钮
 * 由 AFPSPlayerController::Client_ShowRespawnUI 创建
 *
 * 蓝图子类需要:
 *   1. Text Block 命名 KillerNameText
 *   2. Button 命名 RespawnButton
 */
UCLASS(Abstract)
class FPS_API UFPSRespawnWidget : public UFPSBaseMenuWidget
{
	GENERATED_BODY()

public:
	/** 设置击杀者名称 */
	UFUNCTION(BlueprintCallable, Category = "Respawn")
	void SetKillerName(const FString& Name);

protected:
	virtual void NativeConstruct() override;

	// ---------- BindWidget ----------

	UPROPERTY(meta = (BindWidget))
	UTextBlock* KillerNameText;

	UPROPERTY(meta = (BindWidget))
	UButton* RespawnButton;

	/** 击杀信息格式（{0} 替换为击杀者名） */
	UPROPERTY(EditDefaultsOnly, Category = "Respawn")
	FString KillerNameFormat = TEXT("你被 {0} 击杀了");

private:
	UFUNCTION()
	void OnRespawnClicked();
};
