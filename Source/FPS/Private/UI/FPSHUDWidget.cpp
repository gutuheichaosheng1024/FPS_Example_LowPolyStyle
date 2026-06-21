#include "UI/FPSHUDWidget.h"
#include "UI/FPSCrosshairWidget.h"
#include "UI/FPSHitMarkerWidget.h"
#include "UI/FPSKillIndicatorWidget.h"
#include "UI/FPSSaveGame.h"
#include "Kismet/GameplayStatics.h"

UFPSHUDWidget::UFPSHUDWidget()
{
}

// 初始化 HUD，动态创建准星控件并加载保存的配置，首次刷新数据
// 流程：调用 Super::NativeConstruct → 重置刷新计时器 → 创建 CrosshairWidget 并加载已保存的准星配置 → 添加到视口 → 如果启用自动刷新则立即调用 OnRefreshData
void UFPSHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	TimeSinceLastRefresh = 0.f;

	if (CrosshairWidgetClass)
	{
		CrosshairWidget = CreateWidget<UFPSCrosshairWidget>(this, CrosshairWidgetClass);
		if (CrosshairWidget)
		{
			const FCrosshairConfig Saved = UFPSSaveGame::LoadCrosshairConfig();
			CrosshairWidget->ApplyConfig(Saved);

			CrosshairWidget->AddToViewport(0);
		}
	}

	if (bAutoRefresh)
	{
		OnRefreshData();
	}
}

// 每帧 Tick，按 RefreshInterval 定时调用 OnRefreshData 刷新 HUD 数据
// 流程：检查 bAutoRefresh → 累加时间 → 达到 RefreshInterval 时重置计时器并调用 OnRefreshData
void UFPSHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bAutoRefresh) return;

	TimeSinceLastRefresh += InDeltaTime;
	if (TimeSinceLastRefresh >= RefreshInterval)
	{
		TimeSinceLastRefresh = 0.f;
		OnRefreshData();
	}
}

// 蓝图覆盖此事件执行实际数据拉取
// 流程：蓝图子类在此拉取 Character/Weapon/GameMode 等数据并更新 UI 控件
void UFPSHUDWidget::OnRefreshData_Implementation()
{
}

// 手动触发一次立即刷新
// 流程：重置计时器 → 调用 OnRefreshData
void UFPSHUDWidget::RefreshNow()
{
	TimeSinceLastRefresh = 0.f;
	OnRefreshData();
}

// 显示命中提示
// 流程：调用 HitMarkerWidget->ShowHitMarker 触发命中标记出现
void UFPSHUDWidget::ShowHitMarker()
{
	if (HitMarkerWidget)
		HitMarkerWidget->ShowHitMarker();
}

// 显示击杀指示器
// 流程：调用 KillIndicatorWidget->AddKillIcon 追加一个击杀图标
void UFPSHUDWidget::ShowKillIndicator()
{
	if (KillIndicatorWidget)
		KillIndicatorWidget->AddKillIcon();
}

// 播放命中确认音效（仅本地玩家听到）
// 流程：使用 UGameplayStatics::PlaySound2D 播放 2D 音效
void UFPSHUDWidget::PlayHitConfirmSound(USoundBase* Sound)
{
	if (Sound)
		UGameplayStatics::PlaySound2D(this, Sound);
}

// 播放击杀确认音效（仅本地玩家听到）
// 流程：使用 UGameplayStatics::PlaySound2D 播放 2D 音效
void UFPSHUDWidget::PlayKillConfirmSound(USoundBase* Sound)
{
	if (Sound)
		UGameplayStatics::PlaySound2D(this, Sound);
}
