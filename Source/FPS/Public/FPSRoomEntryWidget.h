#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FPSRoomEntryWidget.generated.h"

/** 房间条目点击委托，传递房间索引 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRoomEntryClicked, int32, RoomIndex);

/**
 * 房间列表条目 Widget 基类
 * C++ 负责数据和逻辑，蓝图子类负责布局和视觉
 *
 * 蓝图子类中需要：
 *   1. 放一个 Button 命名 EntryButton
 *   2. 放 Text Block 命名 ServerNameText / MapNameText / PlayersText / PingText
 *   3. 在 EntryButton 的 OnClicked 中调用 HandleButtonClicked()
 */
UCLASS(Abstract)
class FPS_API UFPSRoomEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// ---------- 委托 ----------

	/** 条目被点击时广播，携带 RoomIndex */
	UPROPERTY(BlueprintAssignable, Category = "Room Entry")
	FOnRoomEntryClicked OnRoomEntryClicked;

	// ---------- C++ 逻辑 ----------

	/** 设置房间数据，C++ 处理赋值 */
	void SetRoomData(int32 InIndex, const FString& InServerName, const FString& InMapName,
		int32 InCurrentPlayers, int32 InMaxPlayers, int32 InPing);

	/** 设置选中状态，C++ 更新标记，通知蓝图刷新视觉 */
	void SetSelected(bool bSelected);

	// ---------- Blueprint 可读属性 ----------

	UPROPERTY(BlueprintReadOnly, Category = "Room Entry")
	int32 RoomIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Room Entry")
	FString ServerName;

	UPROPERTY(BlueprintReadOnly, Category = "Room Entry")
	FString MapName;

	UPROPERTY(BlueprintReadOnly, Category = "Room Entry")
	int32 CurrentPlayers = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Room Entry")
	int32 MaxPlayers = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Room Entry")
	int32 PingInMs = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Room Entry")
	bool bIsSelected = false;

	// ---------- Blueprint 可覆盖的视觉更新 ----------

	/** 数据设置后调用，蓝图覆盖此函数更新 Text Block 显示 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Room Entry")
	void OnDataUpdated();

	/** 选中状态变化后调用，蓝图覆盖此函数更新高亮 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Room Entry")
	void OnSelectionChanged(bool bNewSelected);

	// ---------- 按钮点击处理 ----------

	/** 蓝图中 EntryButton 的 OnClicked 调用此函数 */
	UFUNCTION(BlueprintCallable, Category = "Room Entry")
	void HandleButtonClicked();
};
