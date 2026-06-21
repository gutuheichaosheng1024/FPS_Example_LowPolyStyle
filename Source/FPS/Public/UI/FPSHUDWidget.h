#pragma once

#include "CoreMinimal.h"
#include "UI/FPSBaseMenuWidget.h"
#include "FPSHUDWidget.generated.h"

class UFPSCrosshairWidget;
class UFPSHitMarkerWidget;
class UFPSKillIndicatorWidget;
class USoundBase;

/**
 * UFPSHUDWidget — HUD Widget 基类，提供定时刷新机制、命中提示、击杀指示器和准星管理
 *
 * 职责：按 RefreshInterval 定时调用 OnRefreshData 供蓝图覆盖拉取数据；动态创建准星控件并加载保存的配置；管理命中提示和击杀指示器的触发
 * 使用：UFPSBaseMenuWidget、UFPSCrosshairWidget、UFPSHitMarkerWidget、UFPSKillIndicatorWidget、UFPSSaveGame
 */
UCLASS(Abstract, BlueprintType)
class FPS_API UFPSHUDWidget : public UFPSBaseMenuWidget
{
	GENERATED_BODY()

public:
	UFPSHUDWidget();

	UFUNCTION(BlueprintCallable, Category = "HUD|HitMarker")
	void ShowHitMarker();

	UFUNCTION(BlueprintCallable, Category = "HUD|KillIndicator")
	void ShowKillIndicator();

	UFUNCTION(BlueprintCallable, Category = "HUD|HitMarker")
	void PlayHitConfirmSound(USoundBase* Sound);

	UFUNCTION(BlueprintCallable, Category = "HUD|KillIndicator")
	void PlayKillConfirmSound(USoundBase* Sound);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(BlueprintReadOnly, Category = "HUD")
	TObjectPtr<UFPSCrosshairWidget> CrosshairWidget;

	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	TSubclassOf<UFPSCrosshairWidget> CrosshairWidgetClass;

	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "HUD|HitMarker")
	TObjectPtr<UFPSHitMarkerWidget> HitMarkerWidget;

	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "HUD|KillIndicator")
	TObjectPtr<UFPSKillIndicatorWidget> KillIndicatorWidget;

	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	float RefreshInterval = 0.15f;

	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	bool bAutoRefresh = true;

	UFUNCTION(BlueprintNativeEvent, Category = "HUD")
	void OnRefreshData();

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void RefreshNow();

private:
	float TimeSinceLastRefresh = 0.f;
};
