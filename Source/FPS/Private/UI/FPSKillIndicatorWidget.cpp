#include "UI/FPSKillIndicatorWidget.h"
#include "Components/Image.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Spacer.h"
#include "Engine/Texture2D.h"

// 初始化击杀指示器，预生成所有图标并插入到两个 Spacer 之间
// 流程：调用 Super::NativeConstruct → 清空 KillSlots → 循环 MaxKillIcons 次创建 UImage → 设置纹理、大小、初始 Hidden 状态 → 调用 InsertKillIconBetweenSpacers 插入 → 加入 KillSlots → 重置计数器
void UFPSKillIndicatorWidget::NativeConstruct()
{
	Super::NativeConstruct();

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

// 每帧 Tick，处理淡出动画
// 流程：检查 bIsFading → 调用 UpdateFadeOut 更新淡出进度
void UFPSKillIndicatorWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (bIsFading)
	{
		UpdateFadeOut(InDeltaTime);
	}
}

// 将图标插入到 EndSpacer 前面，并设置水平布局属性
// 流程：找到 EndSpacer 在 KillContainer 中的索引 → 在该索引前 InsertChildAt → 设置 HorizontalBoxSlot 的 Padding 和对齐
void UFPSKillIndicatorWidget::InsertKillIconBetweenSpacers(UImage* Icon)
{
	if (!KillContainer || !Icon) return;

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

	KillContainer->InsertChildAt(InsertIndex, Icon);

	if (UHorizontalBoxSlot* BoxSlot = Cast<UHorizontalBoxSlot>(Icon->Slot))
	{
		BoxSlot->SetPadding(FMargin(IconPadding, 0.f));
		BoxSlot->SetHorizontalAlignment(HAlign_Center);
		BoxSlot->SetVerticalAlignment(VAlign_Center);
	}
}

// 添加一个击杀图标，若正在淡出则中断淡出并恢复已显示图标
// 流程：检查是否已满 → 如果正在淡出则停止淡出并恢复已显示图标的透明度 → 显示下一个隐藏的图标 → ActiveKillCount++ → 调用 ResetClearTimer 重置清除计时器
void UFPSKillIndicatorWidget::AddKillIcon()
{
	if (ActiveKillCount >= MaxKillIcons) return;
	if (!KillSlots.IsValidIndex(ActiveKillCount)) return;

	if (bIsFading)
	{
		bIsFading = false;
		FadeElapsed = 0.f;
		for (int32 i = 0; i < ActiveKillCount; i++)
		{
			if (KillSlots[i])
				KillSlots[i]->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 1.f));
		}
	}

	KillSlots[ActiveKillCount]->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 1.f));
	KillSlots[ActiveKillCount]->SetVisibility(ESlateVisibility::HitTestInvisible);
	ActiveKillCount++;

	ResetClearTimer();
}

// 清除所有击杀图标，重置所有状态
// 流程：遍历 KillSlots 将所有图标设置为 Hidden 并恢复透明度 → 重置 ActiveKillCount 和淡出状态 → 清除计时器
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

// 重置清除计时器，ClearDelay 秒后触发 StartFadeOut
// 流程：停止淡出状态 → 清理已存在的计时器 → 设置新计时器在 ClearDelay 后调用 StartFadeOut
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

// 开始淡出动画
// 流程：设置 bIsFading = true，重置 FadeElapsed
void UFPSKillIndicatorWidget::StartFadeOut()
{
	bIsFading = true;
	FadeElapsed = 0.f;
}

// 更新淡出进度，按 FadeDuration 线性降低所有已显示图标的透明度
// 流程：累加 FadeElapsed → 计算 Alpha → 遍历 ActiveKillCount 个已显示图标设置透明度 → Alpha 归零时隐藏所有图标并重置 ActiveKillCount
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
		for (int32 i = 0; i < ActiveKillCount; i++)
		{
			if (KillSlots[i])
				KillSlots[i]->SetVisibility(ESlateVisibility::Hidden);
		}
		ActiveKillCount = 0;
	}
}
