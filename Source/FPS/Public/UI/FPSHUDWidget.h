#pragma once

#include "CoreMinimal.h"
#include "UI/FPSBaseMenuWidget.h"
#include "FPSHUDWidget.generated.h"

class UFPSCrosshairWidget;
class UFPSHitMarkerWidget;
class UFPSKillIndicatorWidget;
class USoundBase;

/**
 * HUD Widget 基类（纯流程）
 *
 * 提供定时刷新机制，不依赖任何具体角色/武器/GameMode 类型。
 * 蓝图子类覆盖 OnRefreshData 事件自行拉取数据并更新 UI。
 *
 * 用法：
 *   1. 蓝图继承此类
 *   2. 覆盖 Event OnRefreshData
 *   3. 在事件体内用 Cast / GetXXX 节点拉取任意数据 → 更新控件
 */
UCLASS(Abstract, BlueprintType)
class FPS_API UFPSHUDWidget : public UFPSBaseMenuWidget
{
	GENERATED_BODY()

public:
	UFPSHUDWidget();

	/** 显示命中提示 */
	UFUNCTION(BlueprintCallable, Category = "HUD|HitMarker")
	void ShowHitMarker();

	/** 显示击杀指示器（追加一个图标） */
	UFUNCTION(BlueprintCallable, Category = "HUD|KillIndicator")
	void ShowKillIndicator();

	/** 播放命中确认音效（仅本地玩家听到） */
	UFUNCTION(BlueprintCallable, Category = "HUD|HitMarker")
	void PlayHitConfirmSound(USoundBase* Sound);

	/** 播放击杀确认音效（仅本地玩家听到） */
	UFUNCTION(BlueprintCallable, Category = "HUD|KillIndicator")
	void PlayKillConfirmSound(USoundBase* Sound);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** 准星控件（C++ 动态创建，不在 Designer 拖放） */
	UPROPERTY(BlueprintReadOnly, Category = "HUD")
	TObjectPtr<UFPSCrosshairWidget> CrosshairWidget;

	/** 准星控件类（蓝图 ClassDefaults 中设置） */
	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	TSubclassOf<UFPSCrosshairWidget> CrosshairWidgetClass;

	// ---------- 命中提示（蓝图 Designer 中放置，BindWidget 绑定） ----------

	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "HUD|HitMarker")
	TObjectPtr<UFPSHitMarkerWidget> HitMarkerWidget;

	// ---------- 击杀指示器（蓝图 Designer 中放置，BindWidget 绑定） ----------

	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "HUD|KillIndicator")
	TObjectPtr<UFPSKillIndicatorWidget> KillIndicatorWidget;

	// ---------- 配置 ----------

	/** 刷新间隔（秒） */
	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	float RefreshInterval = 0.15f;

	/** 启用定时刷新 */
	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	bool bAutoRefresh = true;

	/**
	 * 数据刷新事件（BlueprintNativeEvent）
	 * 在 NativeTick 中按 RefreshInterval 定时调用
	 * 蓝图覆盖此事件，在此拉取 Character/Weapon/GameMode 等数据并更新控件
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "HUD")
	void OnRefreshData();

	/** 手动触发一次刷新 */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void RefreshNow();

private:
	float TimeSinceLastRefresh = 0.f;
};
