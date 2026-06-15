#include "FPSMainMenuWidget.h"
#include "FPSGameInstance.h"
#include "FPSRoomEntryWidget.h"
#include "Components/VerticalBox.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

void UFPSMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	UE_LOG(LogTemp, Log, TEXT("MainMenuWidget: NativeConstruct called"));

	// 诊断：检查 BindWidget 绑定
	UE_LOG(LogTemp, Log, TEXT("  RoomListContainer=%s"), RoomListContainer ? TEXT("OK") : TEXT("NULL"));
	UE_LOG(LogTemp, Log, TEXT("  CreateButton=%s"), CreateButton ? TEXT("OK") : TEXT("NULL"));
	UE_LOG(LogTemp, Log, TEXT("  JoinButton=%s"), JoinButton ? TEXT("OK") : TEXT("NULL"));
	UE_LOG(LogTemp, Log, TEXT("  RefreshButton=%s"), RefreshButton ? TEXT("OK") : TEXT("NULL"));
	UE_LOG(LogTemp, Log, TEXT("  StatusText=%s"), StatusText ? TEXT("OK") : TEXT("NULL"));

	// 获取 GameInstance 并绑定委托
	CachedGameInstance = Cast<UFPSGameInstance>(GetGameInstance());
	UE_LOG(LogTemp, Log, TEXT("  GameInstance=%s"), CachedGameInstance ? TEXT("OK") : TEXT("NULL"));

	if (CachedGameInstance)
	{
		CachedGameInstance->OnRoomListUpdated.AddDynamic(this, &UFPSMainMenuWidget::OnRoomListUpdated);
		CachedGameInstance->OnCreateRoomSuccess.AddDynamic(this, &UFPSMainMenuWidget::OnCreateRoomSuccess);
	}

	// 绑定按钮点击
	if (CreateButton)
		CreateButton->OnClicked.AddDynamic(this, &UFPSMainMenuWidget::OnCreateButtonClicked);
	if (JoinButton)
		JoinButton->OnClicked.AddDynamic(this, &UFPSMainMenuWidget::OnJoinButtonClicked);
	if (RefreshButton)
		RefreshButton->OnClicked.AddDynamic(this, &UFPSMainMenuWidget::OnRefreshButtonClicked);
	if (BackButton)
		BackButton->OnClicked.AddDynamic(this, &UFPSMainMenuWidget::OnBackClicked);

	// 初始状态
	if (JoinButton)
		JoinButton->SetIsEnabled(false);
	if (RefreshButton)
		RefreshButton->SetIsEnabled(false);

	// 首次自动搜索（延迟执行，等待 OnlineSubsystem 就绪）
	if (CachedGameInstance)
	{
		SetStatusText(TEXT("正在搜索房间..."));
		FTimerDelegate TimerDelegate;
		TimerDelegate.BindUObject(this, &UFPSMainMenuWidget::DelayedFindRooms);
		GetWorld()->GetTimerManager().SetTimer(SearchTimerHandle, TimerDelegate, 1.0f, false);
	}
}

void UFPSMainMenuWidget::NativeDestruct()
{
	// 清除延迟搜索定时器
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SearchTimerHandle);
	}

	// 解绑委托
	if (CachedGameInstance)
	{
		CachedGameInstance->OnRoomListUpdated.RemoveDynamic(this, &UFPSMainMenuWidget::OnRoomListUpdated);
		CachedGameInstance->OnCreateRoomSuccess.RemoveDynamic(this, &UFPSMainMenuWidget::OnCreateRoomSuccess);
	}

	Super::NativeDestruct();
}

void UFPSMainMenuWidget::DelayedFindRooms()
{
	if (CachedGameInstance)
	{
		UE_LOG(LogTemp, Log, TEXT("MainMenuWidget: DelayedFindRooms"));
		CachedGameInstance->FindRooms();
	}
}

// ---------- 搜索结果回调 ----------

void UFPSMainMenuWidget::OnRoomListUpdated()
{
	// 搜索结果为 0 时自动重试一次（PIE 时序问题）
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

void UFPSMainMenuWidget::RefreshRoomList()
{
	if (!CachedGameInstance || !RoomListContainer)
	{
		UE_LOG(LogTemp, Error, TEXT("RefreshRoomList: GameInstance=%s, RoomListContainer=%s"),
			CachedGameInstance ? TEXT("OK") : TEXT("NULL"),
			RoomListContainer ? TEXT("OK") : TEXT("NULL"));
		return;
	}

	// 清空旧条目
	RoomListContainer->ClearChildren();
	RoomEntries.Empty();
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

		// 设置数据
		Entry->SetRoomData(
			i,
			RoomList[i].ServerName,
			RoomList[i].MapName,
			RoomList[i].CurrentPlayers,
			RoomList[i].MaxPlayers,
			RoomList[i].PingInMs
		);

		// 绑定点击事件
		Entry->OnRoomEntryClicked.AddDynamic(this, &UFPSMainMenuWidget::OnRoomEntryClicked);
		UE_LOG(LogTemp, Log, TEXT("  Bound OnRoomEntryClicked for room %d"), i);

		// 添加到列表
		RoomListContainer->AddChild(Entry);
		RoomEntries.Add(Entry);
	}

	if (RefreshButton)
		RefreshButton->SetIsEnabled(true);

	SetStatusText(FString::Printf(TEXT("找到 %d 个房间"), RoomList.Num()));

	// 通知蓝图层更新（如有需要）
	OnRoomListUpdatedBP();
}

// ---------- 条目点击 ----------

void UFPSMainMenuWidget::OnRoomEntryClicked(int32 RoomIndex)
{
	UE_LOG(LogTemp, Log, TEXT("MainMenu: OnRoomEntryClicked, RoomIndex=%d"), RoomIndex);
	SelectedRoomIndex = RoomIndex;

	// 更新所有条目的选中状态
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

// ---------- 创建房间成功 ----------

void UFPSMainMenuWidget::OnCreateRoomSuccess()
{
	SetStatusText(TEXT("房间创建成功，正在进入..."));
}

// ---------- 按钮点击 ----------

void UFPSMainMenuWidget::OnCreateButtonClicked()
{
	OpenSubScreen(CreateRoomScreenClass);
}

void UFPSMainMenuWidget::OnJoinButtonClicked()
{
	if (!CachedGameInstance || SelectedRoomIndex < 0)
	{
		SetStatusText(TEXT("请先选择一个房间"));
		return;
	}

	CachedGameInstance->JoinRoom(SelectedRoomIndex);
	SetStatusText(TEXT("正在加入房间..."));

	if (JoinButton)
		JoinButton->SetIsEnabled(false);
}

void UFPSMainMenuWidget::OnRefreshButtonClicked()
{
	if (!CachedGameInstance) return;

	SetStatusText(TEXT("正在搜索房间..."));

	if (RefreshButton)
		RefreshButton->SetIsEnabled(false);

	// 延迟搜索，避免与已有搜索冲突
	FTimerDelegate TimerDelegate;
	TimerDelegate.BindUObject(this, &UFPSMainMenuWidget::DelayedFindRooms);
	GetWorld()->GetTimerManager().SetTimer(SearchTimerHandle, TimerDelegate, 0.5f, false);
}

void UFPSMainMenuWidget::OnBackClicked()
{
	CloseSelf();
}

// ---------- 工具 ----------

void UFPSMainMenuWidget::SetStatusText(const FString& Text)
{
	if (StatusText)
	{
		StatusText->SetText(FText::FromString(Text));
	}
}
