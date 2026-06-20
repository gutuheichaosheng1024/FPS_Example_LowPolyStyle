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
    MapFullPathList.Empty();

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
    TArray<FString> MapPaths;
    TArray<FString> MapDisplayNames;
    for (const FAssetData& AssetData : AssetDataList)
    {
        if (AssetData.AssetClassPath.GetAssetName() == TEXT("World"))
        {
            FString FullPath = AssetData.GetSoftObjectPath().ToString();
            FString DisplayName = ExtractMapName(FullPath);
            if (!DisplayName.IsEmpty() && !ShouldExclude(DisplayName))
            {
                MapPaths.Add(FullPath);
                MapDisplayNames.Add(DisplayName);
            }
        }
    }

    // 按显示名排序（保持路径与显示名同步）
    TArray<int32> Indices;
    for (int32 i = 0; i < MapDisplayNames.Num(); i++) Indices.Add(i);
    Indices.Sort([&MapDisplayNames](int32 A, int32 B) {
        return MapDisplayNames[A] < MapDisplayNames[B];
    });

    // 填充下拉框：显示短名，完整路径存入 MapFullPathList
    if (Indices.Num() > 0)
    {
        for (int32 Idx : Indices)
        {
            MapFullPathList.Add(MapPaths[Idx]);
            MapComboBox->AddOption(MapDisplayNames[Idx]);
        }
        MapComboBox->SetSelectedOption(MapDisplayNames[Indices[0]]);
    }
    else if (!DefaultMapName.IsEmpty())
    {
        // 回退到默认地图
        FString DefaultDisplayName = ExtractMapName(DefaultMapName);
        MapFullPathList.Add(DefaultMapName);
        MapComboBox->AddOption(DefaultDisplayName);
        MapComboBox->SetSelectedOption(DefaultDisplayName);
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

    // 通过 ComboBox 选中索引查找完整资产路径
    FString MapFullPath = DefaultMapName;
    if (MapComboBox)
    {
        int32 SelectedIndex = MapComboBox->GetSelectedIndex();
        if (MapFullPathList.IsValidIndex(SelectedIndex))
        {
            MapFullPath = MapFullPathList[SelectedIndex];
        }
    }

    UFPSGameInstance* GI = Cast<UFPSGameInstance>(GetGameInstance());
    if (GI)
    {
        GI->CreateRoomWithMap(RoomName, MapFullPath);
    }
}

void UFPSCreateRoomScreen::OnBackClicked()
{
    CloseSelf();
}
