#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FPSHitMarkerWidget.generated.h"

class UImage;

/**
 * 命中提示控件
 * 屏幕中心 X 形标记（4个UImage对角排列，中心不相连）
 * 命中时出现，经过 FadeDuration 后透明度降为0隐藏
 * 连续命中重置透明度和计时器
 *
 * 蓝图子类需要:
 *   1. 在 Designer 中放置 4 个 Image 控件
 *   2. 命名为 TopLeftBar / TopRightBar / BottomLeftBar / BottomRightBar
 *   3. 设置 Image 纹理（白色方块或自定义 X 形纹理）
 */
UCLASS(Abstract, BlueprintType)
class FPS_API UFPSHitMarkerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// ======================================================================
	// 数据接口
	// ======================================================================

	/** 显示命中提示（重置透明度，启动淡出） */
	UFUNCTION(BlueprintCallable, Category = "HitMarker")
	void ShowHitMarker();

	// ======================================================================
	// 配置属性（蓝图 ClassDefaults 中设置）
	// ======================================================================

	/** 每条长度 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HitMarker",
		meta = (ClampMin = "2.0", ClampMax = "50.0"))
	float BarLength = 10.f;

	/** 每条粗细 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HitMarker",
		meta = (ClampMin = "1.0", ClampMax = "20.0"))
	float BarThickness = 2.f;

	/** 中心间距（X 中心不相连的距离） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HitMarker",
		meta = (ClampMin = "0.0", ClampMax = "30.0"))
	float BarGap = 8.f;

	/** 标记颜色 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HitMarker")
	FLinearColor MarkerColor = FLinearColor::White;

	/** 淡出时间（秒） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HitMarker",
		meta = (ClampMin = "0.1", ClampMax = "3.0"))
	float FadeDuration = 0.5f;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// ======================================================================
	// BindWidget — 蓝图中必须存在同名控件
	// ======================================================================

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> TopLeftBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> TopRightBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> BottomLeftBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> BottomRightBar;

private:
	/** 更新4个条的位置和大小 */
	void UpdateMarkerLayout();

	// 淡出状态
	float FadeElapsed = 0.f;
	bool bIsFading = false;
};
