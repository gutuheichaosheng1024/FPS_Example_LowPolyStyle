#include "UI/FPSHUDWidget.h"

UFPSHUDWidget::UFPSHUDWidget()
{
}

void UFPSHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	TimeSinceLastRefresh = 0.f;

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
