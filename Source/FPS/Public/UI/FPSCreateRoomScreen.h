#pragma once

#include "CoreMinimal.h"
#include "UI/FPSBaseMenuWidget.h"
#include "FPSCreateRoomScreen.generated.h"

class UEditableTextBox;
class UComboBoxString;
class UButton;

/**
 * 创建房间界面
 * 输入房间名、选择地图、创建房间
 * 地图列表自动扫描指定目录
 */
UCLASS(Abstract)
class FPS_API UFPSCreateRoomScreen : public UFPSBaseMenuWidget
{
    GENERATED_BODY()

protected:
    /** 房间名输入框 */
    UPROPERTY(meta = (BindWidget))
    UEditableTextBox* RoomNameInput;

    /** 地图选择下拉框 */
    UPROPERTY(meta = (BindWidget))
    UComboBoxString* MapComboBox;

    /** 创建按钮 */
    UPROPERTY(meta = (BindWidget))
    UButton* CreateButton;

    /** 返回按钮 */
    UPROPERTY(meta = (BindWidget))
    UButton* BackButton;

    /** 地图扫描目录（Content 相对路径，如 "/Game/MyAsset/Maps"） */
    UPROPERTY(EditDefaultsOnly, Category = "Maps")
    FString MapScanPath = TEXT("/Game/MyAsset/Maps");

    /** 扫描子目录 */
    UPROPERTY(EditDefaultsOnly, Category = "Maps")
    bool bScanSubDirectories = true;

    /** 排除的地图名关键词（如 "MainMenu"、"Test"） */
    UPROPERTY(EditDefaultsOnly, Category = "Maps")
    TArray<FString> ExcludeKeywords;

    /** 默认地图完整路径（列表为空时的回退） */
    UPROPERTY(EditDefaultsOnly, Category = "Maps")
    FString DefaultMapName = TEXT("/Game/MyAsset/Maps/Lvl_Shooter");

    virtual void NativeConstruct() override;

private:
    /** 扫描目录下的所有 World 资产并填充下拉框 */
    void ScanAndPopulateMaps();

    /** 从资产路径提取短地图名（如 "/Game/MyAsset/Maps/Lvl_Shooter" → "Lvl_Shooter"） */
    FString ExtractMapName(const FString& AssetPath) const;

    /** 地图完整路径列表（与 ComboBox 选项索引一一对应） */
    TArray<FString> MapFullPathList;

    /** 检查地图名是否应被排除 */
    bool ShouldExclude(const FString& MapName) const;

    UFUNCTION()
    void OnCreateClicked();

    UFUNCTION()
    void OnBackClicked();
};
