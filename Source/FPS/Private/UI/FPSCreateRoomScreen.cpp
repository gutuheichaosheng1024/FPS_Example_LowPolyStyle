#include "UI/FPSCreateRoomScreen.h"
#include "FPSGameInstance.h"
#include "Components/EditableTextBox.h"
#include "Components/ComboBoxString.h"
#include "Components/Button.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/World.h"

void UFPSCreateRoomScreen::NativeConstruct()
{
    Super::NativeConstruct();

    if (CreateButton)
        CreateButton->OnClicked.AddDynamic(this, &UFPSCreateRoomScreen::OnCreateClicked);
    if (BackButton)
        BackButton->OnClicked.AddDynamic(this, &UFPSCreateRoomScreen::OnBackClicked);

    ScanAndPopulateMaps();

    // 默认房间名
    if (RoomNameInput && RoomNameInput->GetText().IsEmpty())
    {
        RoomNameInput->SetText(FText::FromString(TEXT("我的房间")));
    }
}

void UFPSCreateRoomScreen::ScanAndPopulateMaps()
{
    if (!MapComboBox) return;

    MapComboBox->ClearOptions();

    // 使用 Asset Registry 扫描 World 资产
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

    // 构建扫描路径
    FString ScanPath = MapScanPath;
    if (!ScanPath.StartsWith(TEXT("/Game/")))
    {
        ScanPath = TEXT("/Game/") + ScanPath;
    }

    // 扫描 World 类型的资产
    TArray<FAssetData> AssetDataList;

    if (bScanSubDirectories)
    {
        AssetRegistry.GetAssetsByPath(FName(*ScanPath), AssetDataList, true);
    }
    else
    {
        AssetRegistry.GetAssetsByPath(FName(*ScanPath), AssetDataList, false);
    }

    // 过滤出 World 资产
    TArray<FString> MapNames;
    for (const FAssetData& AssetData : AssetDataList)
    {
        if (AssetData.AssetClassPath.GetAssetName() == TEXT("World"))
        {
            FString MapName = ExtractMapName(AssetData.GetSoftObjectPath().ToString());
            if (!MapName.IsEmpty() && !ShouldExclude(MapName))
            {
                MapNames.Add(MapName);
            }
        }
    }

    // 按字母顺序排序
    MapNames.Sort();

    // 填充下拉框
    if (MapNames.Num() > 0)
    {
        for (const FString& Map : MapNames)
        {
            MapComboBox->AddOption(Map);
        }
        MapComboBox->SetSelectedOption(MapNames[0]);
    }
    else if (!DefaultMapName.IsEmpty())
    {
        // 回退到默认地图
        MapComboBox->AddOption(DefaultMapName);
        MapComboBox->SetSelectedOption(DefaultMapName);
    }
}

FString UFPSCreateRoomScreen::ExtractMapName(const FString& AssetPath) const
{
    // 从 "/Game/MyAsset/Maps/Lvl_Shooter" 提取 "Lvl_Shooter"
    FString Name = FPaths::GetBaseFilename(AssetPath);

    // 去掉可能的后缀
    Name = Name.Replace(TEXT("_BuiltData"), TEXT(""));

    return Name;
}

bool UFPSCreateRoomScreen::ShouldExclude(const FString& MapName) const
{
    for (const FString& Keyword : ExcludeKeywords)
    {
        if (MapName.Contains(Keyword))
        {
            return true;
        }
    }
    return false;
}

void UFPSCreateRoomScreen::OnCreateClicked()
{
    FString RoomName = RoomNameInput ? RoomNameInput->GetText().ToString() : TEXT("我的房间");
    if (RoomName.IsEmpty()) RoomName = TEXT("我的房间");

    FString MapName = MapComboBox ? MapComboBox->GetSelectedOption() : DefaultMapName;
    if (MapName.IsEmpty()) MapName = DefaultMapName;

    UFPSGameInstance* GI = Cast<UFPSGameInstance>(GetGameInstance());
    if (GI)
    {
        GI->CreateRoomWithMap(RoomName, MapName);
    }
}

void UFPSCreateRoomScreen::OnBackClicked()
{
    CloseSelf();
}
