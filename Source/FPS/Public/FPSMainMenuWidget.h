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
 * 主菜单 Widget 基类
 * C++ 负责数据绑定和事件处理，蓝图子类负责布局
 *
 * 蓝图子类中需要：
 *   1. Vertical Box 命名 RoomListContainer
 *   2. Button 命名 CreateButton / JoinButton / RefreshButton
 *   3. Text Block 命名 StatusText
 */
UCLASS(Abstract)
class FPS_API UFPSMainMenuWidget : public UFPSBaseMenuWidget
{
	GENERATED_BODY()

public:
	// ---------- 蓝图可设置的属性 ----------

	/** 房间条目 Widget 类，在蓝图 Class Defaults 中设置 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UFPSRoomEntryWidget> RoomEntryClass;

	/** 创建房间界面类（蓝图 ClassDefaults 中设置） */
	UPROPERTY(EditDefaultsOnly, Category = "Navigation")
	TSubclassOf<UUserWidget> CreateRoomScreenClass;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// ---------- BindWidget：蓝图中必须存在同名控件 ----------

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

	// ---------- Blueprint 可覆盖的回调 ----------

	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void OnRoomListUpdatedBP();

private:
	// ---------- 事件处理（C++ 内部） ----------

	UFUNCTION()
	void OnRoomListUpdated();

	UFUNCTION()
	void OnCreateRoomSuccess();

	UFUNCTION()
	void OnRoomEntryClicked(int32 RoomIndex);

	void RefreshRoomList();
	void SetStatusText(const FString& Text);

	// ---------- 按钮点击 ----------

	UFUNCTION()
	void OnCreateButtonClicked();

	UFUNCTION()
	void OnJoinButtonClicked();

	UFUNCTION()
	void OnRefreshButtonClicked();

	UFUNCTION()
	void OnBackClicked();

	// ---------- 延迟搜索 ----------

	UFUNCTION()
	void DelayedFindRooms();

	// ---------- 状态 ----------

	int32 SelectedRoomIndex = -1;
	int32 RetryCount = 0;
	FTimerHandle SearchTimerHandle;

	/** RoomList 索引 → SessionSearch->SearchResults 索引的映射 */
	TArray<int32> ResultToSearchIndex;

	UPROPERTY()
	UFPSGameInstance* CachedGameInstance = nullptr;

	UPROPERTY()
	TArray<UFPSRoomEntryWidget*> RoomEntries;
};
