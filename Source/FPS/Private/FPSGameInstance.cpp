#include "FPSGameInstance.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Online/OnlineSessionNames.h"
#include "Kismet/GameplayStatics.h"

// Init：初始化 OnlineSubsystem 并绑定 Session 委托
// 流程：调用 Super → 获取 IOnlineSubsystem → 获取 SessionInterface → BindSessionDelegates → 记录日志
void UFPSGameInstance::Init()
{
	Super::Init();

	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		SessionInterface = OnlineSub->GetSessionInterface();
		BindSessionDelegates();
		UE_LOG(LogTemp, Warning, TEXT("FPSGameInstance: OnlineSubsystem '%s' initialized | SessionInterface=%s"),
			*OnlineSub->GetSubsystemName().ToString(),
			SessionInterface.IsValid() ? TEXT("VALID") : TEXT("NULL"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("FPSGameInstance: No OnlineSubsystem found! Check DefaultPlatformService in DefaultEngine.ini"));
	}
}

// BindSessionDelegates：注册 Create/Find/Join/Destroy 四个 Session 异步回调委托
// 流程：检查 SessionInterface → 逐个创建 FDelegate 并 Add_Handle 注册到 SessionInterface
void UFPSGameInstance::BindSessionDelegates()
{
	if (!SessionInterface.IsValid()) return;

	CreateSessionCompleteDelegate = FOnCreateSessionCompleteDelegate::CreateUObject(
		this, &UFPSGameInstance::OnCreateSessionComplete);
	CreateSessionCompleteHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(
		CreateSessionCompleteDelegate);

	FindSessionsCompleteDelegate = FOnFindSessionsCompleteDelegate::CreateUObject(
		this, &UFPSGameInstance::OnFindSessionsComplete);
	FindSessionsCompleteHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(
		FindSessionsCompleteDelegate);

	JoinSessionCompleteDelegate = FOnJoinSessionCompleteDelegate::CreateUObject(
		this, &UFPSGameInstance::OnJoinSessionComplete);
	JoinSessionCompleteHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(
		JoinSessionCompleteDelegate);

	DestroySessionCompleteDelegate = FOnDestroySessionCompleteDelegate::CreateUObject(
		this, &UFPSGameInstance::OnDestroySessionComplete);
	DestroySessionCompleteHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(
		DestroySessionCompleteDelegate);
}

// CreateRoom：创建房间（默认 Lvl_Shooter 地图），带已有 Session 销毁与重入防护
// 流程：设置 CurrentMapName → 检查 SessionInterface → 若操作进行中则排队 → 若有旧 Session 则先销毁 → 否则执行 ExecuteCreateSession
void UFPSGameInstance::CreateRoom(const FString& ServerName)
{
	CurrentMapName = TEXT("/Game/MyAsset/Maps/Lvl_Shooter");

	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("CreateRoom: SessionInterface invalid"));
		return;
	}

	if (bSessionOperationPending)
	{
		PendingServerName = ServerName;
		PendingMapName = CurrentMapName;
		UE_LOG(LogTemp, Warning, TEXT("CreateRoom: Session operation pending, queued"));
		return;
	}

	if (SessionInterface->GetNamedSession(NAME_GameSession))
	{
		PendingServerName = ServerName;
		PendingMapName = CurrentMapName;
		bSessionOperationPending = true;
		SessionInterface->DestroySession(NAME_GameSession);
		return;
	}

	ExecuteCreateSession(ServerName);
}

// CreateRoomWithMap：创建房间（指定地图），逻辑与 CreateRoom 相同
// 流程：设置 CurrentMapName → 重入防护/旧 Session 销毁 → ExecuteCreateSession
void UFPSGameInstance::CreateRoomWithMap(const FString& ServerName, const FString& MapName)
{
	CurrentMapName = MapName;

	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("CreateRoom: SessionInterface invalid"));
		return;
	}

	if (bSessionOperationPending)
	{
		PendingServerName = ServerName;
		PendingMapName = MapName;
		UE_LOG(LogTemp, Warning, TEXT("CreateRoomWithMap: Session operation pending, queued"));
		return;
	}

	if (SessionInterface->GetNamedSession(NAME_GameSession))
	{
		PendingServerName = ServerName;
		PendingMapName = MapName;
		bSessionOperationPending = true;
		SessionInterface->DestroySession(NAME_GameSession);
		return;
	}

	ExecuteCreateSession(ServerName);
}

// ExecuteCreateSession：设置 LAN Session 参数并调用 CreateSession
// 流程：构建 FOnlineSessionSettings → 设置 SETTING_MAPNAME 和 SERVERNAME → 标记 bSessionOperationPending → 调用 SessionInterface->CreateSession
void UFPSGameInstance::ExecuteCreateSession(const FString& ServerName)
{
	FOnlineSessionSettings Settings;
	Settings.NumPublicConnections = 4;
	Settings.bIsLANMatch = true;
	Settings.bShouldAdvertise = true;
	Settings.bAllowJoinInProgress = true;
	Settings.bUsesPresence = false;

	Settings.Set(SETTING_MAPNAME, CurrentMapName,
		EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	Settings.Set(FName("SERVERNAME"), ServerName,
		EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	UE_LOG(LogTemp, Warning, TEXT("ExecuteCreateSession: ServerName='%s' | Map='%s' | bIsLANMatch=%d | bShouldAdvertise=%d"),
		*ServerName, *CurrentMapName, Settings.bIsLANMatch, Settings.bShouldAdvertise);

	bSessionOperationPending = true;

	if (!SessionInterface->CreateSession(0, NAME_GameSession, Settings))
	{
		UE_LOG(LogTemp, Error, TEXT("CreateSession: Failed to start async task"));
		bSessionOperationPending = false;
	}
}

// OnCreateSessionComplete：Session 创建完成回调，广播成功事件并 OpenLevel
// 流程：清除 bSessionOperationPending → 成功则验证 Session → 广播 OnCreateRoomSuccess → OpenLevel 进入游戏地图
void UFPSGameInstance::OnCreateSessionComplete(FName SessionName, bool bSuccess)
{
	UE_LOG(LogTemp, Warning, TEXT("OnCreateSessionComplete: %s, Success=%d"),
		*SessionName.ToString(), bSuccess);

	bSessionOperationPending = false;

	if (bSuccess)
	{
		FNamedOnlineSession* Session = SessionInterface->GetNamedSession(NAME_GameSession);
		if (Session)
		{
			UE_LOG(LogTemp, Warning, TEXT("OnCreateSessionComplete: Session verified | LAN=%d | Advertise=%d | Connections=%d"),
				Session->SessionSettings.bIsLANMatch,
				Session->SessionSettings.bShouldAdvertise,
				Session->SessionSettings.NumPublicConnections);
		}

		OnCreateRoomSuccess.Broadcast();

		UGameplayStatics::OpenLevel(this, FName(*CurrentMapName), true, TEXT("listen"));
	}
}

// FindRooms：搜索 LAN 房间，带搜索中防护和残留 Session 清理
// 流程：检查 SessionInterface → 搜索进行中则忽略 → 有残留 Session 则先销毁并标记 bPendingFindRooms → 否则 ExecuteFindRooms
void UFPSGameInstance::FindRooms()
{
	UE_LOG(LogTemp, Warning, TEXT("FindRooms called"));

	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("FindRooms: SessionInterface invalid"));
		return;
	}

	if (IsSearching())
	{
		UE_LOG(LogTemp, Warning, TEXT("FindRooms: Search in progress, waiting..."));
		return;
	}

	if (!bSessionOperationPending)
	{
		if (SessionInterface->GetNamedSession(NAME_GameSession))
		{
			UE_LOG(LogTemp, Warning, TEXT("FindRooms: Destroying stale session before search, deferring search"));
			PendingServerName.Empty();
			PendingMapName.Empty();
			bSessionOperationPending = true;
			SessionInterface->DestroySession(NAME_GameSession);
			bPendingFindRooms = true;
			return;
		}
	}
	else if (bSessionOperationPending)
	{
		bPendingFindRooms = true;
		UE_LOG(LogTemp, Warning, TEXT("FindRooms: Operation pending, will search after destroy completes"));
		return;
	}

	ExecuteFindRooms();
}

// ExecuteFindRooms：创建 FOnlineSessionSearch 并发起 LAN 搜索
// 流程：Reset SessionSearch → 设置 bIsLanQuery / MaxSearchResults → 调用 FindSessions
void UFPSGameInstance::ExecuteFindRooms()
{
	SessionSearch.Reset();

	SessionSearch = MakeShareable(new FOnlineSessionSearch());
	SessionSearch->bIsLanQuery = true;
	SessionSearch->MaxSearchResults = 20;

	const bool bSuccess = SessionInterface->FindSessions(0, SessionSearch.ToSharedRef());
	UE_LOG(LogTemp, Warning, TEXT("ExecuteFindRooms: bSuccess=%d | MaxResults=%d | bIsLanQuery=%d"),
		bSuccess, SessionSearch->MaxSearchResults, SessionSearch->bIsLanQuery);
}

// IsSearching：检查当前是否有搜索进行中
// 流程：检查 SessionSearch 有效且 SearchState == InProgress
bool UFPSGameInstance::IsSearching() const
{
	return SessionSearch.IsValid() &&
		SessionSearch->SearchState == EOnlineAsyncTaskState::InProgress;
}

// OnFindSessionsComplete：搜索完成回调，读取搜索结果并广播房间列表更新
// 流程：成功则 ReadSearchResults → 广播 OnRoomListUpdated
void UFPSGameInstance::OnFindSessionsComplete(bool bSuccess)
{
	UE_LOG(LogTemp, Warning, TEXT("OnFindSessionsComplete: Success=%d, Results=%d"),
		bSuccess, SessionSearch.IsValid() ? SessionSearch->SearchResults.Num() : 0);

	if (bSuccess)
	{
		ReadSearchResults();
	}

	OnRoomListUpdated.Broadcast();
}

// ReadSearchResults：遍历 SearchResults 提取房间信息，过滤无效条目
// 流程：清空 RoomList → 遍历 SearchResults → 提取 ServerName/MapName/Players/Ping → 过滤无 SERVERNAME 的条目 → 添加到 RoomList
void UFPSGameInstance::ReadSearchResults()
{
	RoomList.Empty();

	if (!SessionSearch.IsValid()) return;

	UE_LOG(LogTemp, Warning, TEXT("ReadSearchResults: %d total results found"), SessionSearch->SearchResults.Num());

	for (int32 i = 0; i < SessionSearch->SearchResults.Num(); i++)
	{
		const FOnlineSessionSearchResult& Result = SessionSearch->SearchResults[i];

		FRoomInfo Info;
		Result.Session.SessionSettings.Get(FName("SERVERNAME"), Info.ServerName);
		Result.Session.SessionSettings.Get(SETTING_MAPNAME, Info.MapName);
		Info.MaxPlayers = Result.Session.SessionSettings.NumPublicConnections;
		Info.CurrentPlayers = Info.MaxPlayers - Result.Session.NumOpenPublicConnections;
		Info.PingInMs = Result.PingInMs;
		Info.ResultIndex = i;

		if (Info.ServerName.IsEmpty())
		{
			UE_LOG(LogTemp, Log, TEXT("  Room[%d]: Skipped (no SERVERNAME) | Map=%s | Ping=%dms"),
				i, *Info.MapName, Info.PingInMs);
			continue;
		}

		RoomList.Add(Info);

		UE_LOG(LogTemp, Warning, TEXT("  Room[%d]: %s | Map=%s | Players=%d/%d | Ping=%dms"),
			i, *Info.ServerName, *Info.MapName, Info.CurrentPlayers, Info.MaxPlayers, Info.PingInMs);
	}

	if (RoomList.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("ReadSearchResults: No valid rooms found (all filtered or empty)"));
	}
}

// JoinRoom：加入指定索引的房间
// 流程：校验 SessionInterface/SessionSearch/索引有效 → 调用 JoinSession
void UFPSGameInstance::JoinRoom(int32 Index)
{
	if (!SessionInterface.IsValid() || !SessionSearch.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("JoinRoom: SessionInterface or SessionSearch invalid"));
		return;
	}

	if (!SessionSearch->SearchResults.IsValidIndex(Index))
	{
		UE_LOG(LogTemp, Error, TEXT("JoinRoom: Invalid index %d"), Index);
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("JoinRoom: Joining room at index %d"), Index);
	SessionInterface->JoinSession(0, NAME_GameSession, SessionSearch->SearchResults[Index]);
}

// OnJoinSessionComplete：加入房间完成回调，获取连接字符串并 ClientTravel
// 流程：检查结果 → 成功则 GetResolvedConnectString → ClientTravel
void UFPSGameInstance::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	UE_LOG(LogTemp, Warning, TEXT("OnJoinSessionComplete: %s, Result=%d"),
		*SessionName.ToString(), static_cast<int32>(Result));

	if (Result != EOnJoinSessionCompleteResult::Success)
	{
		UE_LOG(LogTemp, Error, TEXT("JoinSession failed"));
		return;
	}

	FString ConnectString;
	if (SessionInterface->GetResolvedConnectString(SessionName, ConnectString))
	{
		UE_LOG(LogTemp, Warning, TEXT("Traveling to: %s"), *ConnectString);
		APlayerController* PC = GetFirstLocalPlayerController();
		if (PC)
		{
			PC->ClientTravel(ConnectString, ETravelType::TRAVEL_Absolute);
		}
	}
}

// DestroyCurrentSession：销毁当前 GameSession
// 流程：检查 SessionInterface → 检查 NamedSession 存在 → DestroySession
void UFPSGameInstance::DestroyCurrentSession()
{
	if (!SessionInterface.IsValid()) return;

	if (SessionInterface->GetNamedSession(NAME_GameSession))
	{
		SessionInterface->DestroySession(NAME_GameSession);
	}
}

// OnDestroySessionComplete：Session 销毁完成回调，处理延迟的搜索/创建请求
// 流程：清除 bSessionOperationPending → 优先处理 bPendingFindRooms → 其次处理 PendingServerName 创建请求
void UFPSGameInstance::OnDestroySessionComplete(FName SessionName, bool bSuccess)
{
	UE_LOG(LogTemp, Warning, TEXT("OnDestroySessionComplete: %s, Success=%d"),
		*SessionName.ToString(), bSuccess);

	bSessionOperationPending = false;

	if (bPendingFindRooms)
	{
		bPendingFindRooms = false;
		UE_LOG(LogTemp, Warning, TEXT("OnDestroySessionComplete: Executing deferred FindRooms"));
		ExecuteFindRooms();
		return;
	}

	if (bSuccess && !PendingServerName.IsEmpty())
	{
		FString ServerName = PendingServerName;
		FString MapName = PendingMapName;
		PendingServerName.Empty();
		PendingMapName.Empty();

		CurrentMapName = MapName;
		ExecuteCreateSession(ServerName);
	}
}

// ReturnToMainMenu：返回主菜单，清除所有待处理操作并加载 Lvl_MainMenu
// 流程：清空 Pending 状态 → DestroyCurrentSession → OpenLevel Lvl_MainMenu
void UFPSGameInstance::ReturnToMainMenu()
{
	PendingServerName.Empty();
	PendingMapName.Empty();
	bSessionOperationPending = false;
	bPendingFindRooms = false;

	DestroyCurrentSession();

	UGameplayStatics::OpenLevel(this, FName(TEXT("/Game/MyAsset/Lvl_MainMenu")), true);
}
