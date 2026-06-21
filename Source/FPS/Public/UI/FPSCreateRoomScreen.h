#pragma once

#include "CoreMinimal.h"
#include "UI/FPSBaseMenuWidget.h"
#include "FPSCreateRoomScreen.generated.h"

class UEditableTextBox;
class UComboBoxString;
class UButton;

/**
 * UFPSCreateRoomScreen — 创建房间界面，提供房间名输入、地图选择和创建房间功能
 *
 * 职责：扫描指定目录下的地图资产并填充下拉框；处理地图名排除过滤；将用户选择的房间名和地图路径传递给 GameInstance 创建房间
 * 使用：UFPSBaseMenuWidget、UFPSGameInstance、UEditableTextBox、UComboBoxString、UButton、IAssetRegistry
 */
UCLASS(Abstract)
class FPS_API UFPSCreateRoomScreen : public UFPSBaseMenuWidget
{
    GENERATED_BODY()

protected:
    UPROPERTY(meta = (BindWidget))
    UEditableTextBox* RoomNameInput;

    UPROPERTY(meta = (BindWidget))
    UComboBoxString* MapComboBox;

    UPROPERTY(meta = (BindWidget))
    UButton* CreateButton;

    UPROPERTY(meta = (BindWidget))
    UButton* BackButton;

    UPROPERTY(EditDefaultsOnly, Category = "Maps")
    FString MapScanPath = TEXT("/Game/MyAsset/Maps");

    UPROPERTY(EditDefaultsOnly, Category = "Maps")
    bool bScanSubDirectories = true;

    UPROPERTY(EditDefaultsOnly, Category = "Maps")
    TArray<FString> ExcludeKeywords;

    UPROPERTY(EditDefaultsOnly, Category = "Maps")
    FString DefaultMapName = TEXT("/Game/MyAsset/Maps/Lvl_Shooter");

    virtual void NativeConstruct() override;

private:
    void ScanAndPopulateMaps();
    FString ExtractMapName(const FString& AssetPath) const;
    TArray<FString> MapFullPathList;
    bool ShouldExclude(const FString& MapName) const;

    UFUNCTION()
    void OnCreateClicked();

    UFUNCTION()
    void OnBackClicked();
};
