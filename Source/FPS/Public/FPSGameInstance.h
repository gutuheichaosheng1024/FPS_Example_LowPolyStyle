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

UCLASS()
class FPS_API UFPSGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;

	// ---------- Blueprint 接口 ----------

	UFUNCTION(BlueprintCallable, Category = "Network")
	void CreateRoom(const FString& ServerName);

	UFUNCTION(BlueprintCallable, Category = "Network")
	void FindRooms();

	UFUNCTION(BlueprintCallable, Category = "Network")
	void JoinRoom(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "Network")
	void DestroyCurrentSession();

	// ---------- 委托 ----------

	UPROPERTY(BlueprintAssignable, Category = "Network")
	FOnRoomListUpdated OnRoomListUpdated;

	UPROPERTY(BlueprintAssignable, Category = "Network")
	FOnCreateRoomSuccess OnCreateRoomSuccess;

	// ---------- 数据访问 ----------

	UFUNCTION(BlueprintPure, Category = "Network")
	const TArray<FRoomInfo>& GetRoomList() const { return RoomList; }

	UFUNCTION(BlueprintPure, Category = "Network")
	bool IsSearching() const;

	// ---------- UI 管理 ----------

	/** 获取全局 UI ZOrder 计数器 */
	int32 GetNextZOrder() { return UIZOrderCounter++; }

	/** 创建房间（带地图名） */
	UFUNCTION(BlueprintCallable, Category = "Network")
	void CreateRoomWithMap(const FString& ServerName, const FString& MapName);

	/** 返回主菜单（销毁会话并加载 Lvl_MainMenu） */
	UFUNCTION(BlueprintCallable, Category = "Network")
	void ReturnToMainMenu();

private:
	// ---------- Session 回调 ----------
	void OnCreateSessionComplete(FName SessionName, bool bSuccess);
	void OnFindSessionsComplete(bool bSuccess);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnDestroySessionComplete(FName SessionName, bool bSuccess);

	// ---------- 内部状态 ----------
	void BindSessionDelegates();
	void ExecuteCreateSession(const FString& ServerName);
	void ReadSearchResults();

	IOnlineSessionPtr SessionInterface;
	TSharedPtr<FOnlineSessionSearch> SessionSearch;

	TArray<FRoomInfo> RoomList;

	// Destroy 后延迟创建
	FString PendingServerName;
	FString PendingMapName;
	FString CurrentMapName;

	// UI ZOrder 计数器
	int32 UIZOrderCounter = 0;

	// 委托句柄
	FOnCreateSessionCompleteDelegate CreateSessionCompleteDelegate;
	FOnFindSessionsCompleteDelegate FindSessionsCompleteDelegate;
	FOnJoinSessionCompleteDelegate JoinSessionCompleteDelegate;
	FOnDestroySessionCompleteDelegate DestroySessionCompleteDelegate;

	FDelegateHandle CreateSessionCompleteHandle;
	FDelegateHandle FindSessionsCompleteHandle;
	FDelegateHandle JoinSessionCompleteHandle;
	FDelegateHandle DestroySessionCompleteHandle;
};
