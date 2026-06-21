#include "UI/FPSHitMarkerWidget.h"
#include "Components/Image.h"

// 初始化命中提示控件，更新布局并默认隐藏
// 流程：调用 Super::NativeConstruct → 调用 UpdateMarkerLayout 设置四条 Bar 位置 → 设置为 Hidden
void UFPSHitMarkerWidget::NativeConstruct()
{
	Super::NativeConstruct();
	UpdateMarkerLayout();
	SetVisibility(ESlateVisibility::Hidden);
}

// 每帧 Tick，处理淡出动画
// 流程：检查 bIsFading → 累加 FadeElapsed → 计算当前 Alpha 并设置整体透明度 → Alpha 归零时停止淡出并隐藏
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

// 显示命中提示，重置透明度和淡出计时器
// 流程：重置 FadeElapsed → 设置 bIsFading = true → 恢复完全不透明 → 设置为 HitTestInvisible 可见
void UFPSHitMarkerWidget::ShowHitMarker()
{
	FadeElapsed = 0.f;
	bIsFading = true;
	SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 1.f));
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

// 更新 4 条命中标记的颜色、大小和对角位置
// 流程：计算 Len/Thick/Gap/Offset → 设置四条 Bar 的颜色 → 通过 SetDesiredSizeOverride/SetRenderTransformAngle/SetRenderTranslation 设置大小和旋转偏移
void UFPSHitMarkerWidget::UpdateMarkerLayout()
{
	const float Scale = 1.f;
	const float Len = BarLength * Scale;
	const float Thick = BarThickness * Scale;
	const float Gap = BarGap * Scale;
	const float Offset = Gap * 0.5f + Len * 0.5f;

	if (TopLeftBar) TopLeftBar->SetColorAndOpacity(MarkerColor);
	if (TopRightBar) TopRightBar->SetColorAndOpacity(MarkerColor);
	if (BottomLeftBar) BottomLeftBar->SetColorAndOpacity(MarkerColor);
	if (BottomRightBar) BottomRightBar->SetColorAndOpacity(MarkerColor);

	if (TopLeftBar)
	{
		TopLeftBar->SetDesiredSizeOverride(FVector2D(Thick, Len));
		TopLeftBar->SetRenderTransformAngle(45.f);
		TopLeftBar->SetRenderTranslation(FVector2D(-Offset, -Offset));
	}

	if (TopRightBar)
	{
		TopRightBar->SetDesiredSizeOverride(FVector2D(Thick, Len));
		TopRightBar->SetRenderTransformAngle(-45.f);
		TopRightBar->SetRenderTranslation(FVector2D(Offset, -Offset));
	}

	if (BottomLeftBar)
	{
		BottomLeftBar->SetDesiredSizeOverride(FVector2D(Thick, Len));
		BottomLeftBar->SetRenderTransformAngle(-45.f);
		BottomLeftBar->SetRenderTranslation(FVector2D(-Offset, Offset));
	}

	if (BottomRightBar)
	{
		BottomRightBar->SetDesiredSizeOverride(FVector2D(Thick, Len));
		BottomRightBar->SetRenderTransformAngle(45.f);
		BottomRightBar->SetRenderTranslation(FVector2D(Offset, Offset));
	}
}
