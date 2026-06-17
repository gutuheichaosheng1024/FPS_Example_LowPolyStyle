#include "UI/FPSHUDWidget.h"
#include "UI/FPSCrosshairWidget.h"
#include "UI/FPSSaveGame.h"

UFPSHUDWidget::UFPSHUDWidget()
{
}

void UFPSHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	TimeSinceLastRefresh = 0.f;

	// 动态创建准星控件（一次性加载配置）
	if (CrosshairWidgetClass)
	{
		CrosshairWidget = CreateWidget<UFPSCrosshairWidget>(this, CrosshairWidgetClass);
		if (CrosshairWidget)
		{
			const FCrosshairConfig Saved = UFPSSaveGame::LoadCrosshairConfig();
			CrosshairWidget->ApplyConfig(Saved);

			// 添加到视口（蓝图可在 Designer 中通过 Canvas 占位控制位置）
			CrosshairWidget->AddToViewport(0);
		}
	}

	// 首次立即刷新
	if (bAutoRefresh)
	{
		OnRefreshData();
	}
}

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

void UFPSHUDWidget::OnRefreshData_Implementation()
{
	// 蓝图覆盖此事件执行实际数据拉取
}

void UFPSHUDWidget::RefreshNow()
{
	TimeSinceLastRefresh = 0.f;
	OnRefreshData();
}
