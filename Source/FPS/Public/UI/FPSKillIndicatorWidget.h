#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FPSKillIndicatorWidget.generated.h"

class UImage;
class UHorizontalBox;
class USpacer;
class UTexture2D;

/**
 * 击杀指示器控件
 * 蓝图放置 HorizontalBox（命名为 KillContainer）+ 两个 Spacer（命名为 StartSpacer / EndSpacer）
 * C++ 运行时在两个 Spacer 之间动态生成 Image 控件
 * 图标在 ClearDelay 后淡出消失
 *
 * 蓝图子类需要:
 *   1. 在 Designer 中放置 HorizontalBox，命名为 KillContainer
 *   2. 在 KillContainer 内添加两个 Spacer：StartSpacer（第一个子控件）、EndSpacer（最后一个子控件）
 *   3. 在 ClassDefaults 中设置 KillIconTexture 和 MaxKillIcons
 */
UCLASS(Abstract, BlueprintType)
class FPS_API UFPSKillIndicatorWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// ======================================================================
	// 数据接口
	// ======================================================================

	/** 添加一个击杀图标（插入到两个 Spacer 之间，刷新清除计时器） */
	UFUNCTION(BlueprintCallable, Category = "KillIndicator")
	void AddKillIcon();

	/** 清除所有击杀图标 */
	UFUNCTION(BlueprintCallable, Category = "KillIndicator")
	void ClearAll();

	// ======================================================================
	// 配置属性（蓝图 ClassDefaults 中设置）
	// ======================================================================

	/** 击杀图标纹理 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "KillIndicator")
	TObjectPtr<UTexture2D> KillIconTexture;

	/** 最大击杀图标数 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "KillIndicator",
		meta = (ClampMin = "1", ClampMax = "10"))
	int32 MaxKillIcons = 5;

	/** 单个图标大小 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "KillIndicator",
		meta = (ClampMin = "8", ClampMax = "128"))
	float IconSize = 32.f;

	/** 图标间距 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "KillIndicator",
		meta = (ClampMin = "0", ClampMax = "20"))
	float IconPadding = 4.f;

	/** 全部消失延迟（秒，最后一个击杀后开始倒计时） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "KillIndicator",
		meta = (ClampMin = "1.0", ClampMax = "30.0"))
	float ClearDelay = 5.f;

	/** 淡出时间（秒） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "KillIndicator",
		meta = (ClampMin = "0.1", ClampMax = "5.0"))
	float FadeDuration = 0.5f;

	// ======================================================================
	// 状态查询
	// ======================================================================

	UFUNCTION(BlueprintPure, Category = "KillIndicator")
	int32 GetActiveKillCount() const { return ActiveKillCount; }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// ======================================================================
	// BindWidget — 蓝图中放置控件
	// ======================================================================

	/** 图标容器（HorizontalBox） */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> KillContainer;

	/** 起始 Spacer（永远是第一个子控件，用于居中对齐） */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USpacer> StartSpacer;

	/** 结束 Spacer（永远是最后一个子控件，用于居中对齐） */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USpacer> EndSpacer;

private:
	/** 在两个 Spacer 之间插入图标 */
	void InsertKillIconBetweenSpacers(UImage* Icon);

	/** 重置清除计时器 */
	void ResetClearTimer();

	/** 开始淡出所有可见图标 */
	void StartFadeOut();

	/** 更新淡出进度 */
	void UpdateFadeOut(float DeltaTime);

	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> KillSlots;

	int32 ActiveKillCount = 0;
	FTimerHandle ClearTimerHandle;

	// 淡出状态
	bool bIsFading = false;
	float FadeElapsed = 0.f;
};
