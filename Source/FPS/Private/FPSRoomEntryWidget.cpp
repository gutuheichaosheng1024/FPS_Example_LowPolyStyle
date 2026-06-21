#include "FPSRoomEntryWidget.h"
#include "Misc/Paths.h"

// SetRoomData：设置房间条目的所有数据，从完整路径提取短地图名后通知蓝图刷新
// 流程：赋值 RoomIndex/ServerName/CurrentPlayers/MaxPlayers/PingInMs → MapName 用 FPaths::GetBaseFilename 提取短名 → 调用 OnDataUpdated
void UFPSRoomEntryWidget::SetRoomData(int32 InIndex, const FString& InServerName,
	const FString& InMapName, int32 InCurrentPlayers, int32 InMaxPlayers, int32 InPing)
{
	RoomIndex = InIndex;
	ServerName = InServerName;

	MapName = FPaths::GetBaseFilename(InMapName);

	CurrentPlayers = InCurrentPlayers;
	MaxPlayers = InMaxPlayers;
	PingInMs = InPing;

	OnDataUpdated();
}

// SetSelected：设置条目选中状态，仅在状态变化时通知蓝图刷新高亮
// 流程：检查 bIsSelected 是否变化 → 更新 bIsSelected → 调用 OnSelectionChanged
void UFPSRoomEntryWidget::SetSelected(bool bSelected)
{
	if (bIsSelected == bSelected) return;
	bIsSelected = bSelected;
	UE_LOG(LogTemp, Log, TEXT("RoomEntry: SetSelected=%d, Index=%d"), bSelected, RoomIndex);
	OnSelectionChanged(bSelected);
}

// HandleButtonClicked：按钮点击处理，广播 RoomIndex 给主菜单
// 流程：记录日志 → 广播 OnRoomEntryClicked(RoomIndex)
void UFPSRoomEntryWidget::HandleButtonClicked()
{
	UE_LOG(LogTemp, Log, TEXT("RoomEntry: ButtonClicked, RoomIndex=%d"), RoomIndex);
	OnRoomEntryClicked.Broadcast(RoomIndex);
}
