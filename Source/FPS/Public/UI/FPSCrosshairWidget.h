#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FPSCrosshairWidget.generated.h"

class UImage;

/**
 * 准星控件
 *
 * 自包含，不依赖任何项目类。添加到 BP_GameHUD 的 Canvas Panel 中即可。
 * 运行时可通过蓝图随时切换纹理/颜色/大小。
 *
 * 蓝图用法：
 *   1. 在 BP_GameHUD 的 Designer 中拖入此控件（作为 UserWidget 子项或直接放 Canvas 中）
 *   2. 居中放置（Position X/Y=0.5, Alignment=0.5,0.5）
 *   3. 运行时调用 SetCrosshairTexture / SetCrosshairColor / SetCrosshairScale
 */
UCLASS(Abstract, BlueprintType)
class FPS_API UFPSCrosshairWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFPSCrosshairWidget();

	// ======================================================================
	// 蓝图可调用 — 运行时动态修改
	// ======================================================================

	/** 切换准星纹理 */
	UFUNCTION(BlueprintCallable, Category = "Crosshair")
	void SetCrosshairTexture(UTexture2D* Texture);

	/** 设置准星颜色（含透明度） */
	UFUNCTION(BlueprintCallable, Category = "Crosshair")
	void SetCrosshairColor(FLinearColor Color);

	/** 设置准星缩放（1.0=原始大小） */
	UFUNCTION(BlueprintCallable, Category = "Crosshair")
	void SetCrosshairScale(float Scale);

	/** 设置准星可见性 */
	UFUNCTION(BlueprintCallable, Category = "Crosshair")
	void SetCrosshairVisible(bool bVisible);

	/** 一次性设置所有属性 */
	UFUNCTION(BlueprintCallable, Category = "Crosshair")
	void ConfigureCrosshair(UTexture2D* Texture, FLinearColor Color, float Scale);

	// ======================================================================
	// 可配置默认值
	// ======================================================================

	/** 默认准星纹理 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crosshair")
	TObjectPtr<UTexture2D> DefaultTexture;

	/** 默认颜色 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crosshair")
	FLinearColor DefaultColor = FLinearColor::Green;

	/** 默认缩放 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crosshair")
	float DefaultScale = 1.0f;

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> CrosshairImage;
};
