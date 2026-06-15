#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FPSBaseMenuWidget.generated.h"

class UFPSGameInstance;

/**
 * 菜单 Widget 公共基类
 * 提供子窗口缓存、父窗口导航、ZOrder 管理
 */
UCLASS(Abstract)
class FPS_API UFPSBaseMenuWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** 父窗口引用（由 OpenSubScreen 自动设置） */
    UPROPERTY(BlueprintReadOnly, Category = "Navigation")
    UUserWidget* CachedParent = nullptr;

    /** 打开子窗口（自动缓存、设置父引用、管理 ZOrder） */
    UFUNCTION(BlueprintCallable, Category = "Navigation")
    void OpenSubScreen(TSubclassOf<UUserWidget> SubScreenClass);

    /** 关闭自身，显示父窗口 */
    UFUNCTION(BlueprintCallable, Category = "Navigation")
    void CloseSelf();

protected:
    /** 已打开的子窗口缓存 */
    UPROPERTY()
    TMap<TSubclassOf<UUserWidget>, UUserWidget*> CachedSubScreens;

    /** 获取全局 ZOrder 计数器 */
    int32 GetNextZOrder();
};
