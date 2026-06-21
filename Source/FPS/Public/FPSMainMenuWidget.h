#pragma once

#include "CoreMinimal.h"
#include "UI/FPSBaseMenuWidget.h"
#include "FPSMainMenuWidget.generated.h"

class UFPSRoomEntryWidget;
class UFPSGameInstance;
class UVerticalBox;
class UButton;
class UTextBlock;

/**
 * UFPSMainMenuWidget — 主菜单大厅界面，管理房间列表显示与创建/加入/刷新操作
 *
 * 职责：绑定 UFPSGameInstance 的房间列表委托与创建房间成功委托、动态创建 UFPSRoomEntryWidget 列表、
 *       按钮点击路由（创建房间/加入房间/刷新/返回）、搜索重试逻辑、房间索引到 SearchResults 的映射
 * 使用：UFPSGameInstance, UFPSRoomEntryWidget, UVerticalBox, UButton, UTextBlock
 */
UCLASS(Abstract)
class FPS_API UFPSMainMenuWidget : public UFPSBaseMenuWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UFPSRoomEntryWidget> RoomEntryClass;

	UPROPERTY(EditDefaultsOnly, Category = "Navigation")
	TSubclassOf<UUserWidget> CreateRoomScreenClass;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	UVerticalBox* RoomListContainer = nullptr;

	UPROPERTY(meta = (BindWidget))
	UButton* CreateButton = nullptr;

	UPROPERTY(meta = (BindWidget))
	UButton* JoinButton = nullptr;

	UPROPERTY(meta = (BindWidget))
	UButton* RefreshButton = nullptr;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* StatusText = nullptr;

	UPROPERTY(meta = (BindWidget))
	UButton* BackButton = nullptr;

	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void OnRoomListUpdatedBP();

private:
	UFUNCTION()
	void OnRoomListUpdated();

	UFUNCTION()
	void OnCreateRoomSuccess();

	UFUNCTION()
	void OnRoomEntryClicked(int32 RoomIndex);

	void RefreshRoomList();
	void SetStatusText(const FString& Text);

	UFUNCTION()
	void OnCreateButtonClicked();

	UFUNCTION()
	void OnJoinButtonClicked();

	UFUNCTION()
	void OnRefreshButtonClicked();

	UFUNCTION()
	void OnBackClicked();

	UFUNCTION()
	void DelayedFindRooms();

	int32 SelectedRoomIndex = -1;
	int32 RetryCount = 0;
	FTimerHandle SearchTimerHandle;

	TArray<int32> ResultToSearchIndex;

	UPROPERTY()
	UFPSGameInstance* CachedGameInstance = nullptr;

	UPROPERTY()
	TArray<UFPSRoomEntryWidget*> RoomEntries;
};
