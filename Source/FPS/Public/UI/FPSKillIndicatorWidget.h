#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FPSKillIndicatorWidget.generated.h"

class UImage;
class UHorizontalBox;
class USpacer;
class UTexture2D;

/**
 * UFPSKillIndicatorWidget — 击杀指示器控件，动态生成击杀图标并在延迟后淡出消失
 *
 * 职责：在蓝图 Designer 放置的 HorizontalBox 中两个 Spacer 之间动态插入 Image 图标；每击杀追加一个图标并重置清除计时器；ClearDelay 后开始淡出，FadeDuration 内透明度降至 0 后隐藏
 * 使用：UUserWidget、UImage、UHorizontalBox、USpacer、UTexture2D、FTimerHandle
 */
UCLASS(Abstract, BlueprintType)
class FPS_API UFPSKillIndicatorWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "KillIndicator")
	void AddKillIcon();

	UFUNCTION(BlueprintCallable, Category = "KillIndicator")
	void ClearAll();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "KillIndicator")
	TObjectPtr<UTexture2D> KillIconTexture;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "KillIndicator",
		meta = (ClampMin = "1", ClampMax = "10"))
	int32 MaxKillIcons = 5;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "KillIndicator",
		meta = (ClampMin = "8", ClampMax = "128"))
	float IconSize = 32.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "KillIndicator",
		meta = (ClampMin = "0", ClampMax = "20"))
	float IconPadding = 4.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "KillIndicator",
		meta = (ClampMin = "1.0", ClampMax = "30.0"))
	float ClearDelay = 5.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "KillIndicator",
		meta = (ClampMin = "0.1", ClampMax = "5.0"))
	float FadeDuration = 0.5f;

	UFUNCTION(BlueprintPure, Category = "KillIndicator")
	int32 GetActiveKillCount() const { return ActiveKillCount; }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> KillContainer;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USpacer> StartSpacer;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USpacer> EndSpacer;

private:
	void InsertKillIconBetweenSpacers(UImage* Icon);
	void ResetClearTimer();
	void StartFadeOut();
	void UpdateFadeOut(float DeltaTime);

	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> KillSlots;

	int32 ActiveKillCount = 0;
	FTimerHandle ClearTimerHandle;

	bool bIsFading = false;
	float FadeElapsed = 0.f;
};
