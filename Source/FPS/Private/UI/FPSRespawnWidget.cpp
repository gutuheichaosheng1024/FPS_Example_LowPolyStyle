#include "UI/FPSRespawnWidget.h"
#include "FPSPlayerController.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"

// 初始化重生界面，绑定重生按钮点击事件
// 流程：调用 Super::NativeConstruct → 绑定 RespawnButton 的 OnClicked 到 OnRespawnClicked
void UFPSRespawnWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (RespawnButton)
	{
		RespawnButton->OnClicked.AddDynamic(this, &UFPSRespawnWidget::OnRespawnClicked);
	}
}

// 设置击杀者名称并按格式模板替换显示
// 流程：将 KillerNameFormat 中的 {0} 替换为 Name → SetText 到 KillerNameText
void UFPSRespawnWidget::SetKillerName(const FString& Name)
{
	if (KillerNameText)
	{
		FString Text = KillerNameFormat.Replace(TEXT("{0}"), *Name);
		KillerNameText->SetText(FText::FromString(Text));
	}
}

// 重生按钮回调，在客户端恢复游戏输入模式并请求服务端重生
// 流程：Cast 到 AFPSPlayerController → 设置游戏输入模式并隐藏鼠标 → 调用 Server_RequestRespawn → RemoveFromParent 移除重生界面
void UFPSRespawnWidget::OnRespawnClicked()
{
	if (AFPSPlayerController* PC = Cast<AFPSPlayerController>(GetOwningPlayer()))
	{
		PC->SetInputMode(FInputModeGameOnly());
		PC->SetShowMouseCursor(false);

		PC->Server_RequestRespawn();
	}

	RemoveFromParent();
}
