#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FPSHitMarkerWidget.generated.h"

class UImage;

/**
 * UFPSHitMarkerWidget — 命中提示控件，屏幕中心 X 形标记，命中时出现并淡出消失
 *
 * 职责：4 个 UImage 对角排列形成 X 形命中标记；ShowHitMarker 触发显示并重置淡出计时器；NativeTick 中按 FadeDuration 计算透明度直至隐藏
 * 使用：UUserWidget、UImage
 */
UCLASS(Abstract, BlueprintType)
class FPS_API UFPSHitMarkerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "HitMarker")
	void ShowHitMarker();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HitMarker",
		meta = (ClampMin = "2.0", ClampMax = "50.0"))
	float BarLength = 10.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HitMarker",
		meta = (ClampMin = "1.0", ClampMax = "20.0"))
	float BarThickness = 2.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HitMarker",
		meta = (ClampMin = "0.0", ClampMax = "30.0"))
	float BarGap = 8.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HitMarker")
	FLinearColor MarkerColor = FLinearColor::White;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HitMarker",
		meta = (ClampMin = "0.1", ClampMax = "3.0"))
	float FadeDuration = 0.5f;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> TopLeftBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> TopRightBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> BottomLeftBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> BottomRightBar;

private:
	void UpdateMarkerLayout();

	float FadeElapsed = 0.f;
	bool bIsFading = false;
};
