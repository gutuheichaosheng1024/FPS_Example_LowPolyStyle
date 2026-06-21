#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FPSRoomEntryWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRoomEntryClicked, int32, RoomIndex);

/**
 * UFPSRoomEntryWidget — 房间列表条目 Widget，C++ 负责数据逻辑，蓝图负责布局视觉
 *
 * 职责：存储房间数据（ServerName/MapName/Players/Ping）、处理点击事件并广播 RoomIndex、
 *       管理选中状态并通知蓝图刷新高亮、从完整路径提取短地图名
 * 使用：无特定依赖（继承自 UUserWidget）
 */
UCLASS(Abstract)
class FPS_API UFPSRoomEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Room Entry")
	FOnRoomEntryClicked OnRoomEntryClicked;

	void SetRoomData(int32 InIndex, const FString& InServerName, const FString& InMapName,
		int32 InCurrentPlayers, int32 InMaxPlayers, int32 InPing);

	void SetSelected(bool bSelected);

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

	UFUNCTION(BlueprintImplementableEvent, Category = "Room Entry")
	void OnDataUpdated();

	UFUNCTION(BlueprintImplementableEvent, Category = "Room Entry")
	void OnSelectionChanged(bool bNewSelected);

	UFUNCTION(BlueprintCallable, Category = "Room Entry")
	void HandleButtonClicked();
};
