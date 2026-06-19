#include "UI/FPSKillIndicatorWidget.h"
#include "Components/Image.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Spacer.h"
#include "Engine/Texture2D.h"

void UFPSKillIndicatorWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 预生成所有图标（Hidden 状态），插入到两个 Spacer 之间
	KillSlots.Empty();
	for (int32 i = 0; i < MaxKillIcons; i++)
	{
		UImage* Img = NewObject<UImage>(this, *FString::Printf(TEXT("KillSlot_%d"), i));
		if (!Img) continue;

		if (KillIconTexture)
		{
			Img->SetBrushFromTexture(KillIconTexture);
		}
		Img->SetDesiredSizeOverride(FVector2D(IconSize, IconSize));
		Img->SetVisibility(ESlateVisibility::Hidden);
		Img->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 1.f));

		InsertKillIconBetweenSpacers(Img);
		KillSlots.Add(Img);
	}

	ActiveKillCount = 0;
	bIsFading = false;
}

void UFPSKillIndicatorWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (bIsFading)
	{
		UpdateFadeOut(InDeltaTime);
	}
}

void UFPSKillIndicatorWidget::InsertKillIconBetweenSpacers(UImage* Icon)
{
	if (!KillContainer || !Icon) return;

	// 找到 EndSpacer 的索引，将图标插入到它前面
	int32 InsertIndex = KillContainer->GetChildrenCount();
	if (EndSpacer)
	{
		for (int32 i = 0; i < KillContainer->GetChildrenCount(); i++)
		{
			if (KillContainer->GetChildAt(i) == EndSpacer)
			{
				InsertIndex = i;
				break;
			}
		}
	}

	// 使用 InsertChildAt 将图标插入到 EndSpacer 前面
	KillContainer->InsertChildAt(InsertIndex, Icon);

	// 设置 slot 属性
	if (UHorizontalBoxSlot* BoxSlot = Cast<UHorizontalBoxSlot>(Icon->Slot))
	{
		BoxSlot->SetPadding(FMargin(IconPadding, 0.f));
		BoxSlot->SetHorizontalAlignment(HAlign_Center);
		BoxSlot->SetVerticalAlignment(VAlign_Center);
	}
}

void UFPSKillIndicatorWidget::AddKillIcon()
{
	if (ActiveKillCount >= MaxKillIcons) return;
	if (!KillSlots.IsValidIndex(ActiveKillCount)) return;

	// 如果正在淡出，先停止淡出并恢复当前图标
	if (bIsFading)
	{
		bIsFading = false;
		FadeElapsed = 0.f;
		// 恢复所有已显示图标的透明度
		for (int32 i = 0; i < ActiveKillCount; i++)
		{
			if (KillSlots[i])
				KillSlots[i]->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 1.f));
		}
	}

	// 显示下一个隐藏的图标
	KillSlots[ActiveKillCount]->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 1.f));
	KillSlots[ActiveKillCount]->SetVisibility(ESlateVisibility::HitTestInvisible);
	ActiveKillCount++;

	// 重置清除计时器
	ResetClearTimer();
}

void UFPSKillIndicatorWidget::ClearAll()
{
	for (UImage* Img : KillSlots)
	{
		if (Img)
		{
			Img->SetVisibility(ESlateVisibility::Hidden);
			Img->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 1.f));
		}
	}
	ActiveKillCount = 0;
	bIsFading = false;
	FadeElapsed = 0.f;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ClearTimerHandle);
	}
}

void UFPSKillIndicatorWidget::ResetClearTimer()
{
	bIsFading = false;
	FadeElapsed = 0.f;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ClearTimerHandle);
		World->GetTimerManager().SetTimer(
			ClearTimerHandle, this, &UFPSKillIndicatorWidget::StartFadeOut, ClearDelay, false);
	}
}

void UFPSKillIndicatorWidget::StartFadeOut()
{
	bIsFading = true;
	FadeElapsed = 0.f;
}

void UFPSKillIndicatorWidget::UpdateFadeOut(float DeltaTime)
{
	FadeElapsed += DeltaTime;
	const float Alpha = FMath::Clamp(1.f - FadeElapsed / FadeDuration, 0.f, 1.f);

	for (int32 i = 0; i < ActiveKillCount; i++)
	{
		if (KillSlots[i])
		{
			KillSlots[i]->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, Alpha));
		}
	}

	if (Alpha <= 0.f)
	{
		bIsFading = false;
		// 淡出完成，隐藏所有图标
		for (int32 i = 0; i < ActiveKillCount; i++)
		{
			if (KillSlots[i])
				KillSlots[i]->SetVisibility(ESlateVisibility::Hidden);
		}
		ActiveKillCount = 0;
	}
}
