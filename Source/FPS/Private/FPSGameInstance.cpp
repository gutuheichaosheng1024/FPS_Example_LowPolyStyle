#include "FPSGameInstance.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Online/OnlineSessionNames.h"
#include "Kismet/GameplayStatics.h"

void UFPSGameInstance::Init()
{
	Super::Init();

	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		SessionInterface = OnlineSub->GetSessionInterface();
		BindSessionDelegates();
		UE_LOG(LogTemp, Log, TEXT("FPSGameInstance: OnlineSubsystem '%s' initialized"),
			*OnlineSub->GetSubsystemName().ToString());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("FPSGameInstance: No OnlineSubsystem found!"));
	}
}

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

// ---------- 创建房间 ----------

void UFPSGameInstance::CreateRoom(const FString& ServerName)
{
	CurrentMapName = TEXT("Lvl_Shooter");

	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("CreateRoom: SessionInterface invalid"));
		return;
	}

	// 如果已有 Session，先销毁
	if (SessionInterface->GetNamedSession(NAME_GameSession))
	{
		PendingServerName = ServerName;
		PendingMapName = CurrentMapName;
		SessionInterface->DestroySession(NAME_GameSession);
		return;
	}

	ExecuteCreateSession(ServerName);
}

// ---------- 创建房间（带地图名） ----------

void UFPSGameInstance::CreateRoomWithMap(const FString& ServerName, const FString& MapName)
{
	CurrentMapName = MapName;

	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("CreateRoom: SessionInterface invalid"));
		return;
	}

	// 如果已有 Session，先销毁
	if (SessionInterface->GetNamedSession(NAME_GameSession))
	{
		PendingServerName = ServerName;
		PendingMapName = MapName;
		SessionInterface->DestroySession(NAME_GameSession);
		return;
	}

	ExecuteCreateSession(ServerName);
}

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

	if (!SessionInterface->CreateSession(0, NAME_GameSession, Settings))
	{
		UE_LOG(LogTemp, Error, TEXT("CreateSession: Failed to start async task"));
	}
}

void UFPSGameInstance::OnCreateSessionComplete(FName SessionName, bool bSuccess)
{
	UE_LOG(LogTemp, Log, TEXT("OnCreateSessionComplete: %s, Success=%d"),
		*SessionName.ToString(), bSuccess);

	if (bSuccess)
	{
		OnCreateRoomSuccess.Broadcast();

		// 使用 CurrentMapName 构建地图路径
		FString MapPath = FString::Printf(TEXT("/Game/MyAsset/%s"), *CurrentMapName);
		UGameplayStatics::OpenLevel(this, FName(*MapPath), true, TEXT("listen"));
	}
}

// ---------- 搜索房间 ----------

void UFPSGameInstance::FindRooms()
{
	UE_LOG(LogTemp, Log, TEXT("FindRooms called"));

	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("FindRooms: SessionInterface invalid"));
		return;
	}

	// 如果有正在进行的搜索，先销毁旧 Session 搜索状态
	if (IsSearching())
	{
		UE_LOG(LogTemp, Warning, TEXT("FindRooms: Search in progress, waiting..."));
		return;
	}

	// 确保没有残留的搜索状态
	if (SessionSearch.IsValid() &&
		SessionSearch->SearchState == EOnlineAsyncTaskState::Failed)
	{
		SessionSearch.Reset();
	}

	SessionSearch = MakeShareable(new FOnlineSessionSearch());
	SessionSearch->bIsLanQuery = true;
	SessionSearch->MaxSearchResults = 20;

	const bool bSuccess = SessionInterface->FindSessions(0, SessionSearch.ToSharedRef());
	UE_LOG(LogTemp, Log, TEXT("FindSessions started: bSuccess=%d, MaxResults=%d, bIsLanQuery=%d"),
		bSuccess, SessionSearch->MaxSearchResults, SessionSearch->bIsLanQuery);
}

bool UFPSGameInstance::IsSearching() const
{
	return SessionSearch.IsValid() &&
		SessionSearch->SearchState == EOnlineAsyncTaskState::InProgress;
}

void UFPSGameInstance::OnFindSessionsComplete(bool bSuccess)
{
	UE_LOG(LogTemp, Log, TEXT("OnFindSessionsComplete: Success=%d, Results=%d"),
		bSuccess, SessionSearch.IsValid() ? SessionSearch->SearchResults.Num() : 0);

	if (bSuccess)
	{
		ReadSearchResults();
	}

	OnRoomListUpdated.Broadcast();
}

void UFPSGameInstance::ReadSearchResults()
{
	RoomList.Empty();

	if (!SessionSearch.IsValid()) return;

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
			Info.ServerName = FString::Printf(TEXT("Server %d"), i + 1);
		}

		RoomList.Add(Info);

		UE_LOG(LogTemp, Log, TEXT("  Room[%d]: %s | Map=%s | Players=%d/%d | Ping=%dms"),
			i, *Info.ServerName, *Info.MapName, Info.CurrentPlayers, Info.MaxPlayers, Info.PingInMs);
	}
}

// ---------- 加入房间 ----------

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

	UE_LOG(LogTemp, Log, TEXT("JoinRoom: Joining room at index %d"), Index);
	SessionInterface->JoinSession(0, NAME_GameSession, SessionSearch->SearchResults[Index]);
}

void UFPSGameInstance::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	UE_LOG(LogTemp, Log, TEXT("OnJoinSessionComplete: %s, Result=%d"),
		*SessionName.ToString(), static_cast<int32>(Result));

	if (Result != EOnJoinSessionCompleteResult::Success)
	{
		UE_LOG(LogTemp, Error, TEXT("JoinSession failed"));
		return;
	}

	FString ConnectString;
	if (SessionInterface->GetResolvedConnectString(SessionName, ConnectString))
	{
		UE_LOG(LogTemp, Log, TEXT("Traveling to: %s"), *ConnectString);
		APlayerController* PC = GetFirstLocalPlayerController();
		if (PC)
		{
			PC->ClientTravel(ConnectString, ETravelType::TRAVEL_Absolute);
		}
	}
}

// ---------- 销毁 Session ----------

void UFPSGameInstance::DestroyCurrentSession()
{
	if (!SessionInterface.IsValid()) return;

	if (SessionInterface->GetNamedSession(NAME_GameSession))
	{
		SessionInterface->DestroySession(NAME_GameSession);
	}
}

void UFPSGameInstance::OnDestroySessionComplete(FName SessionName, bool bSuccess)
{
	UE_LOG(LogTemp, Log, TEXT("OnDestroySessionComplete: %s, Success=%d"),
		*SessionName.ToString(), bSuccess);

	// 如果有延迟的创建请求
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
