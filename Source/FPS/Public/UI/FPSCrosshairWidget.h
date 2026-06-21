#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FPSCrosshairWidget.generated.h"

class UImage;
class UOverlay;

UENUM(BlueprintType)
enum class ECrosshairShape : uint8
{
	None        UMETA(DisplayName = "无"),
	Circle      UMETA(DisplayName = "圆形"),
	FourCorner  UMETA(DisplayName = "四角"),
};

USTRUCT(BlueprintType)
struct FPS_API FCrosshairConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shape")
	ECrosshairShape Shape = ECrosshairShape::FourCorner;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CenterDot")
	bool bShowCenterDot = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CenterDot",
		meta = (EditCondition = "bShowCenterDot", ClampMin = "1.0", ClampMax = "20.0"))
	float CenterDotRadius = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CenterDot",
		meta = (EditCondition = "bShowCenterDot"))
	FLinearColor CenterDotColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Circle",
		meta = (EditCondition = "Shape == ECrosshairShape::Circle", ClampMin = "4.0", ClampMax = "100.0"))
	float CircleRadius = 14.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Circle",
		meta = (EditCondition = "Shape == ECrosshairShape::Circle", ClampMin = "1.0", ClampMax = "20.0"))
	float CircleThickness = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Circle",
		meta = (EditCondition = "Shape == ECrosshairShape::Circle"))
	bool bCircleFilled = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FourCorner",
		meta = (EditCondition = "Shape == ECrosshairShape::FourCorner", ClampMin = "1.0", ClampMax = "80.0"))
	float BarLength = 14.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FourCorner",
		meta = (EditCondition = "Shape == ECrosshairShape::FourCorner", ClampMin = "1.0", ClampMax = "20.0"))
	float BarThickness = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FourCorner",
		meta = (EditCondition = "Shape == ECrosshairShape::FourCorner", ClampMin = "0.0", ClampMax = "80.0"))
	float BarGap = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color")
	FLinearColor CrosshairColor = FLinearColor(0.0f, 1.0f, 0.0f, 0.9f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transform",
		meta = (ClampMin = "0.25", ClampMax = "4.0"))
	float OverallScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dynamic")
	bool bRespondToSpread = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dynamic",
		meta = (EditCondition = "bRespondToSpread", ClampMin = "0.1", ClampMax = "5.0"))
	float SpreadRadiusScale = 1.0f;
};

/**
 * UFPSCrosshairWidget — 准星控件，支持四角/圆形/无三种形状，具备动态扩散响应和完整的参数配置
 *
 * 职责：管理准星的形状切换、参数更新和动态扩散；通过 BindWidget 操作蓝图 Designer 中放置的 Image 控件；提供 ApplyConfig/SetSpreadAngle/快捷设置等数据接口
 * 使用：UUserWidget、UImage、UOverlay、FCrosshairConfig
 */
UCLASS(Abstract, BlueprintType)
class FPS_API UFPSCrosshairWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Crosshair")
	void ApplyConfig(const FCrosshairConfig& InConfig);

	UFUNCTION(BlueprintCallable, Category = "Crosshair")
	void SetSpreadAngle(float Angle);

	UFUNCTION(BlueprintPure, Category = "Crosshair")
	const FCrosshairConfig& GetConfig() const { return Config; }

	UFUNCTION(BlueprintPure, Category = "Crosshair")
	float GetCurrentSpreadAngle() const { return CurrentSpreadAngle; }

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

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UOverlay> CrosshairOverlay;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> TopBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> BottomBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> LeftBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> RightBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> CircleRing;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> CenterDot;

private:
	void UpdateAllWidgets();
	void UpdateFourCorner();
	void UpdateCircle();
	void UpdateCenterDot();
	void UpdateShapeVisibility();

	UPROPERTY()
	FCrosshairConfig Config;

	float CurrentSpreadAngle = 0.f;
};
