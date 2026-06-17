#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FPSCrosshairWidget.generated.h"

class UImage;
class UOverlay;

// ======================================================================
// 形状枚举
// ======================================================================

UENUM(BlueprintType)
enum class ECrosshairShape : uint8
{
	None        UMETA(DisplayName = "无"),
	Circle      UMETA(DisplayName = "圆形"),
	FourCorner  UMETA(DisplayName = "四角"),
};

// ======================================================================
// 准星完整配置
// ======================================================================

USTRUCT(BlueprintType)
struct FPS_API FCrosshairConfig
{
	GENERATED_BODY()

	// ---- 形状 ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shape")
	ECrosshairShape Shape = ECrosshairShape::FourCorner;

	// ---- 中心点 ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CenterDot")
	bool bShowCenterDot = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CenterDot",
		meta = (EditCondition = "bShowCenterDot", ClampMin = "1.0", ClampMax = "20.0"))
	float CenterDotRadius = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CenterDot",
		meta = (EditCondition = "bShowCenterDot"))
	FLinearColor CenterDotColor = FLinearColor::White;

	// ---- 圆形参数 ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Circle",
		meta = (EditCondition = "Shape == ECrosshairShape::Circle", ClampMin = "4.0", ClampMax = "100.0"))
	float CircleRadius = 14.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Circle",
		meta = (EditCondition = "Shape == ECrosshairShape::Circle", ClampMin = "1.0", ClampMax = "20.0"))
	float CircleThickness = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Circle",
		meta = (EditCondition = "Shape == ECrosshairShape::Circle"))
	bool bCircleFilled = false;

	// ---- 四角参数 ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FourCorner",
		meta = (EditCondition = "Shape == ECrosshairShape::FourCorner", ClampMin = "1.0", ClampMax = "80.0"))
	float BarLength = 14.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FourCorner",
		meta = (EditCondition = "Shape == ECrosshairShape::FourCorner", ClampMin = "1.0", ClampMax = "20.0"))
	float BarThickness = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FourCorner",
		meta = (EditCondition = "Shape == ECrosshairShape::FourCorner", ClampMin = "0.0", ClampMax = "80.0"))
	float BarGap = 6.0f;

	// ---- 颜色 ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color")
	FLinearColor CrosshairColor = FLinearColor(0.0f, 1.0f, 0.0f, 0.9f);

	// ---- 整体缩放 ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transform",
		meta = (ClampMin = "0.25", ClampMax = "4.0"))
	float OverallScale = 1.0f;

	// ---- 动态扩散响应 ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dynamic")
	bool bRespondToSpread = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dynamic",
		meta = (EditCondition = "bRespondToSpread", ClampMin = "0.1", ClampMax = "5.0"))
	float SpreadRadiusScale = 1.0f;
};

// ======================================================================
// 准星控件（C++ 逻辑 + BindWidget 操作控件）
// ======================================================================

/**
 * 准星控件基类
 * C++ 负责：数据、状态、计算、通过 BindWidget 更新控件
 * 蓝图负责：放置控件、命名、设置纹理
 *
 * 蓝图子类需要：
 *   1. 在 Designer 中放置 Overlay 和 Image 控件
 *   2. 按照下方 BindWidget 命名
 *   3. 设置 Image 纹理
 *   4. 不需要写任何蓝图代码
 */
UCLASS(Abstract, BlueprintType)
class FPS_API UFPSCrosshairWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// ======================================================================
	// 数据接口
	// ======================================================================

	/** 应用完整配置 */
	UFUNCTION(BlueprintCallable, Category = "Crosshair")
	void ApplyConfig(const FCrosshairConfig& InConfig);

	/** 设置当前扩散角度 */
	UFUNCTION(BlueprintCallable, Category = "Crosshair")
	void SetSpreadAngle(float Angle);

	// ======================================================================
	// 状态查询
	// ======================================================================

	UFUNCTION(BlueprintPure, Category = "Crosshair")
	const FCrosshairConfig& GetConfig() const { return Config; }

	UFUNCTION(BlueprintPure, Category = "Crosshair")
	float GetCurrentSpreadAngle() const { return CurrentSpreadAngle; }

	// ======================================================================
	// 快捷设置
	// ======================================================================

	UFUNCTION(BlueprintCallable, Category = "Crosshair|Set")
	void SetShape(ECrosshairShape Shape);

	UFUNCTION(BlueprintCallable, Category = "Crosshair|Set")
	void SetCrosshairColor(FLinearColor Color);

	UFUNCTION(BlueprintCallable, Category = "Crosshair|Set")
	void SetOverallScale(float Scale);

	UFUNCTION(BlueprintCallable, Category = "Crosshair|Set")
	void SetCenterDot(bool bShow, float Radius);

	UFUNCTION(BlueprintCallable, Category = "Crosshair|Set")
	void SetCircleParams(float Radius, float Thickness);

	UFUNCTION(BlueprintCallable, Category = "Crosshair|Set")
	void SetFourCornerParams(float InBarLength, float InBarThickness, float InBarGap);

protected:
	virtual void NativeConstruct() override;

	// ======================================================================
	// BindWidget — 蓝图中必须存在同名控件
	// ======================================================================

	// ---- 根容器 ----
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UOverlay> CrosshairOverlay;

	// ---- 四角控件 ----
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> TopBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> BottomBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> LeftBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> RightBar;

	// ---- 圆形控件 ----
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> CircleRing;

	// ---- 中心点 ----
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> CenterDot;

private:
	/** 更新所有控件 */
	void UpdateAllWidgets();

	/** 更新四角控件 */
	void UpdateFourCorner();

	/** 更新圆形控件 */
	void UpdateCircle();

	/** 更新中心点 */
	void UpdateCenterDot();

	/** 更新形状可见性 */
	void UpdateShapeVisibility();

	// 当前配置
	UPROPERTY()
	FCrosshairConfig Config;

	// 当前扩散角度
	float CurrentSpreadAngle = 0.f;
};
