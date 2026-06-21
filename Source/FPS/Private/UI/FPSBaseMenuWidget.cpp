#include "UI/FPSBaseMenuWidget.h"
#include "FPSGameInstance.h"

// 打开子窗口，若已缓存则复用，否则创建并缓存后显示，同时隐藏自身
// 流程：查找缓存 → 不存在则创建并缓存 → 设置子窗口 CachedParent → 显示子窗口并隐藏自身
void UFPSBaseMenuWidget::OpenSubScreen(TSubclassOf<UUserWidget> SubScreenClass)
{
    if (!SubScreenClass) return;

    UUserWidget** Found = CachedSubScreens.Find(SubScreenClass);
    UUserWidget* Child = Found ? *Found : nullptr;

    if (!Child)
    {
        Child = CreateWidget<UUserWidget>(GetOwningPlayer(), SubScreenClass);
        if (!Child) return;
        CachedSubScreens.Add(SubScreenClass, Child);
    }

    if (UFPSBaseMenuWidget* BaseChild = Cast<UFPSBaseMenuWidget>(Child))
    {
        BaseChild->CachedParent = this;
    }

    Child->SetVisibility(ESlateVisibility::Visible);
    Child->AddToViewport(GetNextZOrder());

    SetVisibility(ESlateVisibility::Collapsed);
}

// 关闭自身，恢复父窗口的显示
// 流程：隐藏自身 → 如果 CachedParent 存在则恢复其显示
void UFPSBaseMenuWidget::CloseSelf()
{
    SetVisibility(ESlateVisibility::Collapsed);

    if (CachedParent)
    {
        CachedParent->SetVisibility(ESlateVisibility::Visible);
        CachedParent->AddToViewport(GetNextZOrder());
    }
}

// 从 GameInstance 获取全局 ZOrder 计数器
// 流程：Cast 到 UFPSGameInstance → 调用 GetNextZOrder → 失败返回 0
int32 UFPSBaseMenuWidget::GetNextZOrder()
{
    if (UFPSGameInstance* GI = Cast<UFPSGameInstance>(GetGameInstance()))
    {
        return GI->GetNextZOrder();
    }
    return 0;
}
