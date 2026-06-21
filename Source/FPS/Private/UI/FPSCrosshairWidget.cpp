#include "UI/FPSCrosshairWidget.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"

// 初始化准星控件，执行首次全量更新
// 流程：调用 Super::NativeConstruct → 调用 UpdateAllWidgets 初始化所有子控件
void UFPSCrosshairWidget::NativeConstruct()
{
	Super::NativeConstruct();
	UpdateAllWidgets();
}

// 应用完整准星配置并刷新所有控件
// 流程：保存 Config → 调用 UpdateAllWidgets 全量刷新
void UFPSCrosshairWidget::ApplyConfig(const FCrosshairConfig& InConfig)
{
	Config = InConfig;
	UpdateAllWidgets();
}

// 设置当前扩散角度并刷新所有控件
// 流程：保存 CurrentSpreadAngle → 调用 UpdateAllWidgets 全量刷新
void UFPSCrosshairWidget::SetSpreadAngle(float Angle)
{
	CurrentSpreadAngle = Angle;
	UpdateAllWidgets();
}

// 快捷设置准星形状
// 流程：更新 Config.Shape → 调用 UpdateAllWidgets
void UFPSCrosshairWidget::SetShape(ECrosshairShape Shape)
{
	Config.Shape = Shape;
	UpdateAllWidgets();
}

// 快捷设置准星颜色
// 流程：更新 Config.CrosshairColor → 调用 UpdateAllWidgets
void UFPSCrosshairWidget::SetCrosshairColor(FLinearColor Color)
{
	Config.CrosshairColor = Color;
	UpdateAllWidgets();
}

// 快捷设置整体缩放
// 流程：Clamp 缩放值到有效范围 → 更新 Config.OverallScale → 调用 UpdateAllWidgets
void UFPSCrosshairWidget::SetOverallScale(float Scale)
{
	Config.OverallScale = FMath::Clamp(Scale, 0.25f, 4.0f);
	UpdateAllWidgets();
}

// 快捷设置中心点显示状态和半径
// 流程：更新 Config.bShowCenterDot 和 Clamp 后的 CenterDotRadius → 调用 UpdateAllWidgets
void UFPSCrosshairWidget::SetCenterDot(bool bShow, float Radius)
{
	Config.bShowCenterDot = bShow;
	Config.CenterDotRadius = FMath::Clamp(Radius, 1.0f, 20.0f);
	UpdateAllWidgets();
}

// 快捷设置圆形参数
// 流程：Clamp CircleRadius 和 CircleThickness → 更新 Config → 调用 UpdateAllWidgets
void UFPSCrosshairWidget::SetCircleParams(float Radius, float Thickness)
{
	Config.CircleRadius = FMath::Clamp(Radius, 4.0f, 100.0f);
	Config.CircleThickness = FMath::Clamp(Thickness, 1.0f, 20.0f);
	UpdateAllWidgets();
}

// 快捷设置四角参数
// 流程：Clamp BarLength/BarThickness/BarGap → 更新 Config → 调用 UpdateAllWidgets
void UFPSCrosshairWidget::SetFourCornerParams(float InBarLength, float InBarThickness, float InBarGap)
{
	Config.BarLength = FMath::Clamp(InBarLength, 1.0f, 80.0f);
	Config.BarThickness = FMath::Clamp(InBarThickness, 1.0f, 20.0f);
	Config.BarGap = FMath::Clamp(InBarGap, 0.0f, 80.0f);
	UpdateAllWidgets();
}

// 全量更新所有子控件
// 流程：调用 UpdateShapeVisibility → UpdateFourCorner → UpdateCircle → UpdateCenterDot
void UFPSCrosshairWidget::UpdateAllWidgets()
{
	UpdateShapeVisibility();
	UpdateFourCorner();
	UpdateCircle();
	UpdateCenterDot();
}

// 根据当前形状切换四角和圆形控件的可见性
// 流程：计算 bShowFourCorner 和 bShowCircle → 设置 TopBar/BottomBar/LeftBar/RightBar 的可见性 → 设置 CircleRing 的可见性
void UFPSCrosshairWidget::UpdateShapeVisibility()
{
	const bool bShowFourCorner = (Config.Shape == ECrosshairShape::FourCorner);
	const bool bShowCircle = (Config.Shape == ECrosshairShape::Circle);

	if (TopBar) TopBar->SetVisibility(bShowFourCorner ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (BottomBar) BottomBar->SetVisibility(bShowFourCorner ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (LeftBar) LeftBar->SetVisibility(bShowFourCorner ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (RightBar) RightBar->SetVisibility(bShowFourCorner ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);

	if (CircleRing) CircleRing->SetVisibility(bShowCircle ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

// 更新四角控件的颜色、大小和位置，处理扩散偏移
// 流程：检查当前形状是否为 FourCorner → 计算带缩放的 BarLen 和 BarThick → 计算扩散偏移 SpreadOffset → 计算 Offset = Gap + 半条长 → 设置四条 Bar 的颜色 → 通过 SetDesiredSizeOverride/SetRenderTranslation 设置大小和位置
void UFPSCrosshairWidget::UpdateFourCorner()
{
	if (Config.Shape != ECrosshairShape::FourCorner) return;

	const float Scale = Config.OverallScale;
	const float BarLen = Config.BarLength * Scale;
	const float BarThick = Config.BarThickness * Scale;

	float SpreadOffset = 0.f;
	if (Config.bRespondToSpread)
	{
		SpreadOffset = CurrentSpreadAngle * Config.SpreadRadiusScale;
	}
	const float Gap = Config.BarGap * Scale + SpreadOffset;
	const float Offset = Gap + BarLen * 0.5f;

	const FLinearColor Color = Config.CrosshairColor;
	if (TopBar) TopBar->SetColorAndOpacity(Color);
	if (BottomBar) BottomBar->SetColorAndOpacity(Color);
	if (LeftBar) LeftBar->SetColorAndOpacity(Color);
	if (RightBar) RightBar->SetColorAndOpacity(Color);

	if (TopBar)
	{
		TopBar->SetDesiredSizeOverride(FVector2D(BarThick, BarLen));
		TopBar->SetRenderTranslation(FVector2D(0.f, -Offset));
	}

	if (BottomBar)
	{
		BottomBar->SetDesiredSizeOverride(FVector2D(BarThick, BarLen));
		BottomBar->SetRenderTranslation(FVector2D(0.f, Offset));
	}

	if (LeftBar)
	{
		LeftBar->SetDesiredSizeOverride(FVector2D(BarLen, BarThick));
		LeftBar->SetRenderTranslation(FVector2D(-Offset, 0.f));
	}

	if (RightBar)
	{
		RightBar->SetDesiredSizeOverride(FVector2D(BarLen, BarThick));
		RightBar->SetRenderTranslation(FVector2D(Offset, 0.f));
	}
}

// 更新圆形控件的颜色、大小，处理扩散偏移
// 流程：检查当前形状是否为 Circle → 计算带缩放和扩散的半径 → 设置颜色 → 通过 SetDesiredSizeOverride 设置圆形直径
void UFPSCrosshairWidget::UpdateCircle()
{
	if (Config.Shape != ECrosshairShape::Circle) return;
	if (!CircleRing) return;

	const float Scale = Config.OverallScale;

	float SpreadOffset = 0.f;
	if (Config.bRespondToSpread)
	{
		SpreadOffset = CurrentSpreadAngle * Config.SpreadRadiusScale;
	}
	const float Radius = Config.CircleRadius * Scale + SpreadOffset;

	CircleRing->SetColorAndOpacity(Config.CrosshairColor);

	CircleRing->SetDesiredSizeOverride(FVector2D(Radius * 2.f, Radius * 2.f));
	CircleRing->SetRenderTranslation(FVector2D(0.f, 0.f));
}

// 更新中心点的可见性、颜色和大小
// 流程：检查 bShowCenterDot → 设置可见性 → 隐藏则直接返回 → 计算带缩放的半径 → 设置颜色和大小
void UFPSCrosshairWidget::UpdateCenterDot()
{
	if (!CenterDot) return;

	CenterDot->SetVisibility(Config.bShowCenterDot ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);

	if (!Config.bShowCenterDot) return;

	const float Scale = Config.OverallScale;
	const float Radius = Config.CenterDotRadius * Scale;

	CenterDot->SetColorAndOpacity(Config.CenterDotColor);

	CenterDot->SetDesiredSizeOverride(FVector2D(Radius * 2.f, Radius * 2.f));
	CenterDot->SetRenderTranslation(FVector2D(0.f, 0.f));
}
