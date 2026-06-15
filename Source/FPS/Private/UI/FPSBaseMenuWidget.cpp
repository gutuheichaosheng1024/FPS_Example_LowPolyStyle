#include "UI/FPSBaseMenuWidget.h"
#include "FPSGameInstance.h"

void UFPSBaseMenuWidget::OpenSubScreen(TSubclassOf<UUserWidget> SubScreenClass)
{
    if (!SubScreenClass) return;

    // 查找缓存
    UUserWidget** Found = CachedSubScreens.Find(SubScreenClass);
    UUserWidget* Child = Found ? *Found : nullptr;

    // 不存在则创建
    if (!Child)
    {
        Child = CreateWidget<UUserWidget>(GetOwningPlayer(), SubScreenClass);
        if (!Child) return;
        CachedSubScreens.Add(SubScreenClass, Child);
    }

    // 设置父引用（通过基类方法，无需反射）
    if (UFPSBaseMenuWidget* BaseChild = Cast<UFPSBaseMenuWidget>(Child))
    {
        BaseChild->CachedParent = this;
    }

    // 显示子窗口
    Child->SetVisibility(ESlateVisibility::Visible);
    Child->AddToViewport(GetNextZOrder());

    // 隐藏自身
    SetVisibility(ESlateVisibility::Collapsed);
}

void UFPSBaseMenuWidget::CloseSelf()
{
    SetVisibility(ESlateVisibility::Collapsed);

    if (CachedParent)
    {
        CachedParent->SetVisibility(ESlateVisibility::Visible);
        CachedParent->AddToViewport(GetNextZOrder());
    }
}

int32 UFPSBaseMenuWidget::GetNextZOrder()
{
    if (UFPSGameInstance* GI = Cast<UFPSGameInstance>(GetGameInstance()))
    {
        return GI->GetNextZOrder();
    }
    return 0;
}
