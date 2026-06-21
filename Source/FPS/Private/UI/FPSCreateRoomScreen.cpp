#include "UI/FPSCreateRoomScreen.h"
#include "FPSGameInstance.h"
#include "Components/EditableTextBox.h"
#include "Components/ComboBoxString.h"
#include "Components/Button.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/World.h"

// 初始化创建房间界面，绑定按钮事件、扫描地图列表、设置默认房间名
// 流程：调用 Super::NativeConstruct → 绑定 CreateButton 和 BackButton 点击事件 → 调用 ScanAndPopulateMaps 扫描地图 → 如果 RoomNameInput 为空则设为默认名称
void UFPSCreateRoomScreen::NativeConstruct()
{
    Super::NativeConstruct();

    if (CreateButton)
        CreateButton->OnClicked.AddDynamic(this, &UFPSCreateRoomScreen::OnCreateClicked);
    if (BackButton)
        BackButton->OnClicked.AddDynamic(this, &UFPSCreateRoomScreen::OnBackClicked);

    ScanAndPopulateMaps();

    if (RoomNameInput && RoomNameInput->GetText().IsEmpty())
    {
        RoomNameInput->SetText(FText::FromString(TEXT("我的房间")));
    }
}

// 通过 Asset Registry 扫描指定路径下的 World 资产并填充下拉框
// 流程：清空 ComboBox 和路径列表 → 加载 AssetRegistry 模块 → 按路径扫描资产 → 过滤 World 类型并排除关键词 → 按显示名排序 → 将短名填入 ComboBox，完整路径存入 MapFullPathList → 无结果时回退到默认地图
void UFPSCreateRoomScreen::ScanAndPopulateMaps()
{
    if (!MapComboBox) return;

    MapComboBox->ClearOptions();
    MapFullPathList.Empty();

    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

    FString ScanPath = MapScanPath;
    if (!ScanPath.StartsWith(TEXT("/Game/")))
    {
        ScanPath = TEXT("/Game/") + ScanPath;
    }

    TArray<FAssetData> AssetDataList;

    if (bScanSubDirectories)
    {
        AssetRegistry.GetAssetsByPath(FName(*ScanPath), AssetDataList, true);
    }
    else
    {
        AssetRegistry.GetAssetsByPath(FName(*ScanPath), AssetDataList, false);
    }

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

    TArray<int32> Indices;
    for (int32 i = 0; i < MapDisplayNames.Num(); i++) Indices.Add(i);
    Indices.Sort([&MapDisplayNames](int32 A, int32 B) {
        return MapDisplayNames[A] < MapDisplayNames[B];
    });

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
        FString DefaultDisplayName = ExtractMapName(DefaultMapName);
        MapFullPathList.Add(DefaultMapName);
        MapComboBox->AddOption(DefaultDisplayName);
        MapComboBox->SetSelectedOption(DefaultDisplayName);
    }
}

// 从资产完整路径提取地图短名
// 流程：调用 FPaths::GetBaseFilename 获取基本文件名 → 去掉 "_BuiltData" 后缀
FString UFPSCreateRoomScreen::ExtractMapName(const FString& AssetPath) const
{
    FString Name = FPaths::GetBaseFilename(AssetPath);

    Name = Name.Replace(TEXT("_BuiltData"), TEXT(""));

    return Name;
}

// 检查地图名是否包含排除关键词
// 流程：遍历 ExcludeKeywords 数组 → 如果 MapName 包含任一关键词则返回 true
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

// 创建按钮回调，读取房间名和地图路径并调用 GameInstance 创建房间
// 流程：获取 RoomNameInput 文本 → 从 ComboBox 选中索引查找完整地图路径 → Cast 到 UFPSGameInstance → 调用 CreateRoomWithMap
void UFPSCreateRoomScreen::OnCreateClicked()
{
    FString RoomName = RoomNameInput ? RoomNameInput->GetText().ToString() : TEXT("我的房间");
    if (RoomName.IsEmpty()) RoomName = TEXT("我的房间");

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

// 返回按钮回调，关闭自身
// 流程：调用基类 CloseSelf
void UFPSCreateRoomScreen::OnBackClicked()
{
    CloseSelf();
}
