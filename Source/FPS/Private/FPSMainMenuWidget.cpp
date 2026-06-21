#include "FPSMainMenuWidget.h"
#include "FPSGameInstance.h"
#include "FPSRoomEntryWidget.h"
#include "Components/VerticalBox.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

// NativeConstruct：初始化大厅界面，绑定委托和按钮事件，首次自动搜索房间
// 流程：调用 Super → 诊断日志检查 BindWidget 绑定 → 获取 UFPSGameInstance 并绑定房间列表/创建成功委托 →
//       绑定按钮 OnClicked → 设置初始按钮状态 → 延迟 1 秒执行 DelayedFindRooms
void UFPSMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	UE_LOG(LogTemp, Log, TEXT("MainMenuWidget: NativeConstruct called"));

	UE_LOG(LogTemp, Log, TEXT("  RoomListContainer=%s"), RoomListContainer ? TEXT("OK") : TEXT("NULL"));
	UE_LOG(LogTemp, Log, TEXT("  CreateButton=%s"), CreateButton ? TEXT("OK") : TEXT("NULL"));
	UE_LOG(LogTemp, Log, TEXT("  JoinButton=%s"), JoinButton ? TEXT("OK") : TEXT("NULL"));
	UE_LOG(LogTemp, Log, TEXT("  RefreshButton=%s"), RefreshButton ? TEXT("OK") : TEXT("NULL"));
	UE_LOG(LogTemp, Log, TEXT("  StatusText=%s"), StatusText ? TEXT("OK") : TEXT("NULL"));

	CachedGameInstance = Cast<UFPSGameInstance>(GetGameInstance());
	UE_LOG(LogTemp, Log, TEXT("  GameInstance=%s"), CachedGameInstance ? TEXT("OK") : TEXT("NULL"));

	if (CachedGameInstance)
	{
		CachedGameInstance->OnRoomListUpdated.AddDynamic(this, &UFPSMainMenuWidget::OnRoomListUpdated);
		CachedGameInstance->OnCreateRoomSuccess.AddDynamic(this, &UFPSMainMenuWidget::OnCreateRoomSuccess);
	}

	if (CreateButton)
		CreateButton->OnClicked.AddDynamic(this, &UFPSMainMenuWidget::OnCreateButtonClicked);
	if (JoinButton)
		JoinButton->OnClicked.AddDynamic(this, &UFPSMainMenuWidget::OnJoinButtonClicked);
	if (RefreshButton)
		RefreshButton->OnClicked.AddDynamic(this, &UFPSMainMenuWidget::OnRefreshButtonClicked);
	if (BackButton)
		BackButton->OnClicked.AddDynamic(this, &UFPSMainMenuWidget::OnBackClicked);

	if (JoinButton)
		JoinButton->SetIsEnabled(false);
	if (RefreshButton)
		RefreshButton->SetIsEnabled(false);

	if (CachedGameInstance)
	{
		SetStatusText(TEXT("正在搜索房间..."));
		FTimerDelegate TimerDelegate;
		TimerDelegate.BindUObject(this, &UFPSMainMenuWidget::DelayedFindRooms);
		GetWorld()->GetTimerManager().SetTimer(SearchTimerHandle, TimerDelegate, 1.0f, false);
	}
}

// NativeDestruct：清理定时器和委托绑定
// 流程：清除 SearchTimerHandle → 解绑 GameInstance 委托 → 调用 Super::NativeDestruct
void UFPSMainMenuWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SearchTimerHandle);
	}

	if (CachedGameInstance)
	{
		CachedGameInstance->OnRoomListUpdated.RemoveDynamic(this, &UFPSMainMenuWidget::OnRoomListUpdated);
		CachedGameInstance->OnCreateRoomSuccess.RemoveDynamic(this, &UFPSMainMenuWidget::OnCreateRoomSuccess);
	}

	Super::NativeDestruct();
}

// DelayedFindRooms：延迟执行房间搜索
// 流程：检查 CachedGameInstance → 调用 FindRooms
void UFPSMainMenuWidget::DelayedFindRooms()
{
	if (CachedGameInstance)
	{
		UE_LOG(LogTemp, Log, TEXT("MainMenuWidget: DelayedFindRooms"));
		CachedGameInstance->FindRooms();
	}
}

// OnRoomListUpdated：房间列表更新回调，结果为空时自动重试一次
// 流程：检查结果数 → 为 0 且 RetryCount < 1 则延迟 2 秒重试 → 否则调用 RefreshRoomList 刷新列表
void UFPSMainMenuWidget::OnRoomListUpdated()
{
	if (CachedGameInstance && CachedGameInstance->GetRoomList().Num() == 0)
	{
		if (RetryCount < 1)
		{
			RetryCount++;
			UE_LOG(LogTemp, Log, TEXT("OnRoomListUpdated: 0 results, retrying (%d)"), RetryCount);
			FTimerDelegate TimerDelegate;
			TimerDelegate.BindUObject(this, &UFPSMainMenuWidget::DelayedFindRooms);
			GetWorld()->GetTimerManager().SetTimer(SearchTimerHandle, TimerDelegate, 2.0f, false);
			return;
		}
	}

	RetryCount = 0;
	RefreshRoomList();
}

// RefreshRoomList：清空旧条目，遍历 RoomList 动态创建 UFPSRoomEntryWidget 填充列表
// 流程：清空 RoomListContainer → 遍历 RoomList → CreateWidget UFPSRoomEntryWidget → SetRoomData →
//       记录 ResultToSearchIndex 映射 → 绑定 OnRoomEntryClicked → AddChild → 更新状态文本和按钮
void UFPSMainMenuWidget::RefreshRoomList()
{
	if (!CachedGameInstance || !RoomListContainer)
	{
		UE_LOG(LogTemp, Error, TEXT("RefreshRoomList: GameInstance=%s, RoomListContainer=%s"),
			CachedGameInstance ? TEXT("OK") : TEXT("NULL"),
			RoomListContainer ? TEXT("OK") : TEXT("NULL"));
		return;
	}

	RoomListContainer->ClearChildren();
	RoomEntries.Empty();
	ResultToSearchIndex.Empty();
	SelectedRoomIndex = -1;

	if (JoinButton)
		JoinButton->SetIsEnabled(false);

	const TArray<FRoomInfo>& RoomList = CachedGameInstance->GetRoomList();
	UE_LOG(LogTemp, Log, TEXT("RefreshRoomList: %d rooms, RoomEntryClass=%s"),
		RoomList.Num(), RoomEntryClass ? *RoomEntryClass->GetName() : TEXT("NULL"));

	for (int32 i = 0; i < RoomList.Num(); i++)
	{
		if (!RoomEntryClass)
		{
			UE_LOG(LogTemp, Error, TEXT("  RoomEntryClass is NULL!"));
			continue;
		}

		UFPSRoomEntryWidget* Entry = CreateWidget<UFPSRoomEntryWidget>(this, RoomEntryClass);
		if (!Entry)
		{
			UE_LOG(LogTemp, Error, TEXT("  CreateWidget failed for room %d"), i);
			continue;
		}

		Entry->SetRoomData(
			i,
			RoomList[i].ServerName,
			RoomList[i].MapName,
			RoomList[i].CurrentPlayers,
			RoomList[i].MaxPlayers,
			RoomList[i].PingInMs
		);

		ResultToSearchIndex.Add(RoomList[i].ResultIndex);

		Entry->OnRoomEntryClicked.AddDynamic(this, &UFPSMainMenuWidget::OnRoomEntryClicked);
		UE_LOG(LogTemp, Log, TEXT("  Bound OnRoomEntryClicked for room %d"), i);

		RoomListContainer->AddChild(Entry);
		RoomEntries.Add(Entry);
	}

	if (RefreshButton)
		RefreshButton->SetIsEnabled(true);

	SetStatusText(FString::Printf(TEXT("找到 %d 个房间"), RoomList.Num()));

	OnRoomListUpdatedBP();
}

// OnRoomEntryClicked：房间条目被点击，更新选中状态并启用加入按钮
// 流程：记录 SelectedRoomIndex → 遍历所有条目更新选中状态 → 启用 JoinButton
void UFPSMainMenuWidget::OnRoomEntryClicked(int32 RoomIndex)
{
	UE_LOG(LogTemp, Log, TEXT("MainMenu: OnRoomEntryClicked, RoomIndex=%d"), RoomIndex);
	SelectedRoomIndex = RoomIndex;

	for (int32 i = 0; i < RoomEntries.Num(); i++)
	{
		if (RoomEntries[i])
		{
			RoomEntries[i]->SetSelected(i == RoomIndex);
		}
	}

	if (JoinButton)
		JoinButton->SetIsEnabled(true);
}

// OnCreateRoomSuccess：房间创建成功回调，更新状态文本
// 流程：SetStatusText 提示进入中
void UFPSMainMenuWidget::OnCreateRoomSuccess()
{
	SetStatusText(TEXT("房间创建成功，正在进入..."));
}

// OnCreateButtonClicked：打开创建房间子界面
// 流程：OpenSubScreen(CreateRoomScreenClass)
void UFPSMainMenuWidget::OnCreateButtonClicked()
{
	OpenSubScreen(CreateRoomScreenClass);
}

// OnJoinButtonClicked：加入选中的房间
// 流程：校验 SelectedRoomIndex 和 ResultToSearchIndex → 通过映射表获取 SearchResults 索引 → 调用 JoinRoom → 禁用按钮
void UFPSMainMenuWidget::OnJoinButtonClicked()
{
	if (!CachedGameInstance || SelectedRoomIndex < 0)
	{
		SetStatusText(TEXT("请先选择一个房间"));
		return;
	}

	if (!ResultToSearchIndex.IsValidIndex(SelectedRoomIndex))
	{
		SetStatusText(TEXT("房间索引无效"));
		return;
	}

	const int32 SearchResultIndex = ResultToSearchIndex[SelectedRoomIndex];
	CachedGameInstance->JoinRoom(SearchResultIndex);
	SetStatusText(TEXT("正在加入房间..."));

	if (JoinButton)
		JoinButton->SetIsEnabled(false);
}

// OnRefreshButtonClicked：手动刷新房间列表
// 流程：检查 CachedGameInstance → 设置状态文本 → 禁用刷新按钮 → 延迟 0.5 秒调用 DelayedFindRooms
void UFPSMainMenuWidget::OnRefreshButtonClicked()
{
	if (!CachedGameInstance) return;

	SetStatusText(TEXT("正在搜索房间..."));

	if (RefreshButton)
		RefreshButton->SetIsEnabled(false);

	FTimerDelegate TimerDelegate;
	TimerDelegate.BindUObject(this, &UFPSMainMenuWidget::DelayedFindRooms);
	GetWorld()->GetTimerManager().SetTimer(SearchTimerHandle, TimerDelegate, 0.5f, false);
}

// OnBackClicked：返回上一级界面
// 流程：CloseSelf
void UFPSMainMenuWidget::OnBackClicked()
{
	CloseSelf();
}

// SetStatusText：设置状态文本，封装空指针检查
// 流程：检查 StatusText 有效性 → SetText
void UFPSMainMenuWidget::SetStatusText(const FString& Text)
{
	if (StatusText)
	{
		StatusText->SetText(FText::FromString(Text));
	}
}
