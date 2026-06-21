#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FPSBaseMenuWidget.generated.h"

class UFPSGameInstance;

/**
 * UFPSBaseMenuWidget — 菜单 Widget 公共基类，提供子窗口缓存、父窗口导航和 ZOrder 管理
 *
 * 职责：管理子窗口的创建与缓存；通过 CachedParent 实现父子窗口导航；委托 GameInstance 分配全局 ZOrder
 * 使用：UFPSGameInstance
 */
UCLASS(Abstract)
class FPS_API UFPSBaseMenuWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadOnly, Category = "Navigation")
    UUserWidget* CachedParent = nullptr;

    UFUNCTION(BlueprintCallable, Category = "Navigation")
    void OpenSubScreen(TSubclassOf<UUserWidget> SubScreenClass);

    UFUNCTION(BlueprintCallable, Category = "Navigation")
    void CloseSelf();

protected:
    UPROPERTY()
    TMap<TSubclassOf<UUserWidget>, UUserWidget*> CachedSubScreens;

    int32 GetNextZOrder();
};
