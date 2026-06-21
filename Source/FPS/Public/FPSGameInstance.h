#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "FPSGameInstance.generated.h"

class FOnlineSessionSearch;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRoomListUpdated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCreateRoomSuccess);

USTRUCT(BlueprintType)
struct FPS_API FRoomInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FString ServerName;

	UPROPERTY(BlueprintReadOnly)
	FString MapName;

	UPROPERTY(BlueprintReadOnly)
	int32 CurrentPlayers = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 MaxPlayers = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 PingInMs = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 ResultIndex = 0;
};

/**
 * UFPSGameInstance — 游戏实例，管理在线 Session（创建/搜索/加入/销毁房间）与 UI ZOrder
 *
 * 职责：LAN Session 生命周期管理（CreateRoom/FindRooms/JoinRoom/DestroySession）、
 *       Session 异步回调处理与重入防护、房间列表缓存与委托广播、UI ZOrder 计数器
 * 使用：IOnlineSessionPtr, FOnlineSessionSearch
 */
UCLASS()
class FPS_API UFPSGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;

	UFUNCTION(BlueprintCallable, Category = "Network")
	void CreateRoom(const FString& ServerName);

	UFUNCTION(BlueprintCallable, Category = "Network")
	void FindRooms();

	UFUNCTION(BlueprintCallable, Category = "Network")
	void JoinRoom(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "Network")
	void DestroyCurrentSession();

	UPROPERTY(BlueprintAssignable, Category = "Network")
	FOnRoomListUpdated OnRoomListUpdated;

	UPROPERTY(BlueprintAssignable, Category = "Network")
	FOnCreateRoomSuccess OnCreateRoomSuccess;

	UFUNCTION(BlueprintPure, Category = "Network")
	const TArray<FRoomInfo>& GetRoomList() const { return RoomList; }

	UFUNCTION(BlueprintPure, Category = "Network")
	bool IsSearching() const;

	int32 GetNextZOrder() { return UIZOrderCounter++; }

	UFUNCTION(BlueprintCallable, Category = "Network")
	void CreateRoomWithMap(const FString& ServerName, const FString& MapName);

	UFUNCTION(BlueprintCallable, Category = "Network")
	void ReturnToMainMenu();

private:
	void OnCreateSessionComplete(FName SessionName, bool bSuccess);
	void OnFindSessionsComplete(bool bSuccess);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnDestroySessionComplete(FName SessionName, bool bSuccess);

	void BindSessionDelegates();
	void ExecuteCreateSession(const FString& ServerName);
	void ExecuteFindRooms();
	void ReadSearchResults();

	IOnlineSessionPtr SessionInterface;
	TSharedPtr<FOnlineSessionSearch> SessionSearch;

	TArray<FRoomInfo> RoomList;

	FString PendingServerName;
	FString PendingMapName;
	FString CurrentMapName;

	int32 UIZOrderCounter = 0;

	bool bSessionOperationPending = false;
	bool bPendingFindRooms = false;

	FOnCreateSessionCompleteDelegate CreateSessionCompleteDelegate;
	FOnFindSessionsCompleteDelegate FindSessionsCompleteDelegate;
	FOnJoinSessionCompleteDelegate JoinSessionCompleteDelegate;
	FOnDestroySessionCompleteDelegate DestroySessionCompleteDelegate;

	FDelegateHandle CreateSessionCompleteHandle;
	FDelegateHandle FindSessionsCompleteHandle;
	FDelegateHandle JoinSessionCompleteHandle;
	FDelegateHandle DestroySessionCompleteHandle;
};
