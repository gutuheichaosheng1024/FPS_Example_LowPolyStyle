#include "UI/FPSCrosshairWidget.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"

UFPSCrosshairWidget::UFPSCrosshairWidget()
{
}

void UFPSCrosshairWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (CrosshairImage)
	{
		// 应用默认值
		if (DefaultTexture)
		{
			CrosshairImage->SetBrushFromTexture(DefaultTexture);
		}
		CrosshairImage->SetColorAndOpacity(DefaultColor);
		CrosshairImage->SetRenderScale(FVector2D(DefaultScale, DefaultScale));
	}
}

void UFPSCrosshairWidget::SetCrosshairTexture(UTexture2D* Texture)
{
	if (CrosshairImage && Texture)
	{
		CrosshairImage->SetBrushFromTexture(Texture);
	}
}

void UFPSCrosshairWidget::SetCrosshairColor(FLinearColor Color)
{
	if (CrosshairImage)
	{
		CrosshairImage->SetColorAndOpacity(Color);
	}
}

void UFPSCrosshairWidget::SetCrosshairScale(float Scale)
{
	if (CrosshairImage)
	{
		CrosshairImage->SetRenderScale(FVector2D(FMath::Max(Scale, 0.01f), FMath::Max(Scale, 0.01f)));
	}
}

void UFPSCrosshairWidget::SetCrosshairVisible(bool bVisible)
{
	if (CrosshairImage)
	{
		CrosshairImage->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}
}

void UFPSCrosshairWidget::ConfigureCrosshair(UTexture2D* Texture, FLinearColor Color, float Scale)
{
	SetCrosshairTexture(Texture);
	SetCrosshairColor(Color);
	SetCrosshairScale(Scale);
}
