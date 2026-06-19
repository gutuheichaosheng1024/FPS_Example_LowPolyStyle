#include "UI/FPSHitMarkerWidget.h"
#include "Components/Image.h"

void UFPSHitMarkerWidget::NativeConstruct()
{
	Super::NativeConstruct();
	UpdateMarkerLayout();
	SetVisibility(ESlateVisibility::Hidden);
}

void UFPSHitMarkerWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!bIsFading) return;

	FadeElapsed += InDeltaTime;
	const float Alpha = FMath::Clamp(1.f - FadeElapsed / FadeDuration, 0.f, 1.f);
	SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, Alpha));

	if (Alpha <= 0.f)
	{
		bIsFading = false;
		SetVisibility(ESlateVisibility::Hidden);
	}
}

void UFPSHitMarkerWidget::ShowHitMarker()
{
	FadeElapsed = 0.f;
	bIsFading = true;
	SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 1.f));
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UFPSHitMarkerWidget::UpdateMarkerLayout()
{
	const float Scale = 1.f;
	const float Len = BarLength * Scale;
	const float Thick = BarThickness * Scale;
	const float Gap = BarGap * Scale;
	const float Offset = Gap * 0.5f + Len * 0.5f;

	// 颜色
	if (TopLeftBar) TopLeftBar->SetColorAndOpacity(MarkerColor);
	if (TopRightBar) TopRightBar->SetColorAndOpacity(MarkerColor);
	if (BottomLeftBar) BottomLeftBar->SetColorAndOpacity(MarkerColor);
	if (BottomRightBar) BottomRightBar->SetColorAndOpacity(MarkerColor);

	// TopLeft: 左上角，对角线方向（-X, -Y）
	if (TopLeftBar)
	{
		TopLeftBar->SetDesiredSizeOverride(FVector2D(Thick, Len));
		TopLeftBar->SetRenderTransformAngle(45.f);
		TopLeftBar->SetRenderTranslation(FVector2D(-Offset, -Offset));
	}

	// TopRight: 右上角（+X, -Y）
	if (TopRightBar)
	{
		TopRightBar->SetDesiredSizeOverride(FVector2D(Thick, Len));
		TopRightBar->SetRenderTransformAngle(-45.f);
		TopRightBar->SetRenderTranslation(FVector2D(Offset, -Offset));
	}

	// BottomLeft: 左下角（-X, +Y）
	if (BottomLeftBar)
	{
		BottomLeftBar->SetDesiredSizeOverride(FVector2D(Thick, Len));
		BottomLeftBar->SetRenderTransformAngle(-45.f);
		BottomLeftBar->SetRenderTranslation(FVector2D(-Offset, Offset));
	}

	// BottomRight: 右下角（+X, +Y）
	if (BottomRightBar)
	{
		BottomRightBar->SetDesiredSizeOverride(FVector2D(Thick, Len));
		BottomRightBar->SetRenderTransformAngle(45.f);
		BottomRightBar->SetRenderTranslation(FVector2D(Offset, Offset));
	}
}
