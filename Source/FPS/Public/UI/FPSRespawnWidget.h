#pragma once

#include "CoreMinimal.h"
#include "UI/FPSBaseMenuWidget.h"
#include "FPSRespawnWidget.generated.h"

class UTextBlock;
class UButton;

/**
 * UFPSRespawnWidget — 重生界面，显示击杀者名称并提供重生按钮
 *
 * 职责：接收击杀者名称并按格式模板显示；重生按钮点击时在客户端恢复游戏输入模式并通过 Server RPC 请求重生；由 AFPSPlayerController::Client_ShowRespawnUI 创建
 * 使用：UFPSBaseMenuWidget、AFPSPlayerController、UTextBlock、UButton
 */
UCLASS(Abstract)
class FPS_API UFPSRespawnWidget : public UFPSBaseMenuWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Respawn")
	void SetKillerName(const FString& Name);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* KillerNameText;

	UPROPERTY(meta = (BindWidget))
	UButton* RespawnButton;

	UPROPERTY(EditDefaultsOnly, Category = "Respawn")
	FString KillerNameFormat = TEXT("你被 {0} 击杀了");

private:
	UFUNCTION()
	void OnRespawnClicked();
};
