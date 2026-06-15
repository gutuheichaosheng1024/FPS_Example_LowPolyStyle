#include "FPSRoomEntryWidget.h"

void UFPSRoomEntryWidget::SetRoomData(int32 InIndex, const FString& InServerName,
	const FString& InMapName, int32 InCurrentPlayers, int32 InMaxPlayers, int32 InPing)
{
	RoomIndex = InIndex;
	ServerName = InServerName;
	MapName = InMapName;
	CurrentPlayers = InCurrentPlayers;
	MaxPlayers = InMaxPlayers;
	PingInMs = InPing;

	OnDataUpdated();
}

void UFPSRoomEntryWidget::SetSelected(bool bSelected)
{
	if (bIsSelected == bSelected) return;
	bIsSelected = bSelected;
	UE_LOG(LogTemp, Log, TEXT("RoomEntry: SetSelected=%d, Index=%d"), bSelected, RoomIndex);
	OnSelectionChanged(bSelected);
}

void UFPSRoomEntryWidget::HandleButtonClicked()
{
	UE_LOG(LogTemp, Log, TEXT("RoomEntry: ButtonClicked, RoomIndex=%d"), RoomIndex);
	OnRoomEntryClicked.Broadcast(RoomIndex);
}
