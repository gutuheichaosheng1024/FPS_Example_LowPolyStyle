#include "UI/FPSCrosshairWidget.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"

// ======================================================================
// 初始化
// ======================================================================

void UFPSCrosshairWidget::NativeConstruct()
{
	Super::NativeConstruct();
	UpdateAllWidgets();
}

// ======================================================================
// 数据接口
// ======================================================================

void UFPSCrosshairWidget::ApplyConfig(const FCrosshairConfig& InConfig)
{
	Config = InConfig;
	UpdateAllWidgets();
}

void UFPSCrosshairWidget::SetSpreadAngle(float Angle)
{
	CurrentSpreadAngle = Angle;
	UpdateAllWidgets();
}

// ======================================================================
// 快捷设置
// ======================================================================

void UFPSCrosshairWidget::SetShape(ECrosshairShape Shape)
{
	Config.Shape = Shape;
	UpdateAllWidgets();
}

void UFPSCrosshairWidget::SetCrosshairColor(FLinearColor Color)
{
	Config.CrosshairColor = Color;
	UpdateAllWidgets();
}

void UFPSCrosshairWidget::SetOverallScale(float Scale)
{
	Config.OverallScale = FMath::Clamp(Scale, 0.25f, 4.0f);
	UpdateAllWidgets();
}

void UFPSCrosshairWidget::SetCenterDot(bool bShow, float Radius)
{
	Config.bShowCenterDot = bShow;
	Config.CenterDotRadius = FMath::Clamp(Radius, 1.0f, 20.0f);
	UpdateAllWidgets();
}

void UFPSCrosshairWidget::SetCircleParams(float Radius, float Thickness)
{
	Config.CircleRadius = FMath::Clamp(Radius, 4.0f, 100.0f);
	Config.CircleThickness = FMath::Clamp(Thickness, 1.0f, 20.0f);
	UpdateAllWidgets();
}

void UFPSCrosshairWidget::SetFourCornerParams(float InBarLength, float InBarThickness, float InBarGap)
{
	Config.BarLength = FMath::Clamp(InBarLength, 1.0f, 80.0f);
	Config.BarThickness = FMath::Clamp(InBarThickness, 1.0f, 20.0f);
	Config.BarGap = FMath::Clamp(InBarGap, 0.0f, 80.0f);
	UpdateAllWidgets();
}

// ======================================================================
// 控件更新
// ======================================================================

void UFPSCrosshairWidget::UpdateAllWidgets()
{
	UpdateShapeVisibility();
	UpdateFourCorner();
	UpdateCircle();
	UpdateCenterDot();
}

void UFPSCrosshairWidget::UpdateShapeVisibility()
{
	const bool bShowFourCorner = (Config.Shape == ECrosshairShape::FourCorner);
	const bool bShowCircle = (Config.Shape == ECrosshairShape::Circle);

	// 四角控件可见性
	if (TopBar) TopBar->SetVisibility(bShowFourCorner ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (BottomBar) BottomBar->SetVisibility(bShowFourCorner ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (LeftBar) LeftBar->SetVisibility(bShowFourCorner ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (RightBar) RightBar->SetVisibility(bShowFourCorner ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);

	// 圆形控件可见性
	if (CircleRing) CircleRing->SetVisibility(bShowCircle ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

void UFPSCrosshairWidget::UpdateFourCorner()
{
	if (Config.Shape != ECrosshairShape::FourCorner) return;

	const float Scale = Config.OverallScale;
	const float BarLen = Config.BarLength * Scale;
	const float BarThick = Config.BarThickness * Scale;

	// 计算偏移（Gap + 扩散）
	float SpreadOffset = 0.f;
	if (Config.bRespondToSpread)
	{
		SpreadOffset = CurrentSpreadAngle * Config.SpreadRadiusScale;
	}
	const float Gap = Config.BarGap * Scale + SpreadOffset;
	const float Offset = Gap + BarLen * 0.5f;

	// 更新颜色
	const FLinearColor Color = Config.CrosshairColor;
	if (TopBar) TopBar->SetColorAndOpacity(Color);
	if (BottomBar) BottomBar->SetColorAndOpacity(Color);
	if (LeftBar) LeftBar->SetColorAndOpacity(Color);
	if (RightBar) RightBar->SetColorAndOpacity(Color);

	// 更新大小和位置（通过 RenderTransform 偏移）
	// TopBar: 上方，水平条
	if (TopBar)
	{
		TopBar->SetDesiredSizeOverride(FVector2D(BarThick, BarLen));
		TopBar->SetRenderTranslation(FVector2D(0.f, -Offset));
	}

	// BottomBar: 下方，水平条
	if (BottomBar)
	{
		BottomBar->SetDesiredSizeOverride(FVector2D(BarThick, BarLen));
		BottomBar->SetRenderTranslation(FVector2D(0.f, Offset));
	}

	// LeftBar: 左侧，垂直条
	if (LeftBar)
	{
		LeftBar->SetDesiredSizeOverride(FVector2D(BarLen, BarThick));
		LeftBar->SetRenderTranslation(FVector2D(-Offset, 0.f));
	}

	// RightBar: 右侧，垂直条
	if (RightBar)
	{
		RightBar->SetDesiredSizeOverride(FVector2D(BarLen, BarThick));
		RightBar->SetRenderTranslation(FVector2D(Offset, 0.f));
	}
}

void UFPSCrosshairWidget::UpdateCircle()
{
	if (Config.Shape != ECrosshairShape::Circle) return;
	if (!CircleRing) return;

	const float Scale = Config.OverallScale;

	// 计算半径（含扩散）
	float SpreadOffset = 0.f;
	if (Config.bRespondToSpread)
	{
		SpreadOffset = CurrentSpreadAngle * Config.SpreadRadiusScale;
	}
	const float Radius = Config.CircleRadius * Scale + SpreadOffset;

	// 更新颜色
	CircleRing->SetColorAndOpacity(Config.CrosshairColor);

	// 更新大小
	CircleRing->SetDesiredSizeOverride(FVector2D(Radius * 2.f, Radius * 2.f));
	CircleRing->SetRenderTranslation(FVector2D(0.f, 0.f));
}

void UFPSCrosshairWidget::UpdateCenterDot()
{
	if (!CenterDot) return;

	// 可见性
	CenterDot->SetVisibility(Config.bShowCenterDot ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);

	if (!Config.bShowCenterDot) return;

	const float Scale = Config.OverallScale;
	const float Radius = Config.CenterDotRadius * Scale;

	// 更新颜色
	CenterDot->SetColorAndOpacity(Config.CenterDotColor);

	// 更新大小
	CenterDot->SetDesiredSizeOverride(FVector2D(Radius * 2.f, Radius * 2.f));
	CenterDot->SetRenderTranslation(FVector2D(0.f, 0.f));
}
