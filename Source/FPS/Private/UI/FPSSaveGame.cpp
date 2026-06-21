#include "UI/FPSSaveGame.h"
#include "Kismet/GameplayStatics.h"

const FString UFPSSaveGame::SaveSlotName = TEXT("FPS_Settings");
const int32 UFPSSaveGame::UserIndex = 0;
static const FString CrosshairSlotName = TEXT("FPS_Crosshair");

// 加载通用设置存档，不存在则返回默认构造的对象
// 流程：检查存档是否存在 → 存在则 LoadGameFromSlot 并 Cast → 不存在或失败则 NewObject 返回默认值
UFPSSaveGame* UFPSSaveGame::LoadSettings()
{
    if (UGameplayStatics::DoesSaveGameExist(SaveSlotName, UserIndex))
    {
        return Cast<UFPSSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlotName, UserIndex));
    }

    return NewObject<UFPSSaveGame>();
}

// 保存通用设置，先加载已有存档以保留 PlayerName，再写入新值并保存
// 流程：调用 LoadSettings 加载已有存档 → 更新 MasterVolume/BackgroundVolume/Resolution/bFullscreen → SaveGameToSlot 保存
void UFPSSaveGame::SaveSettings(float Master, float Background, const FString& Res, bool bInFullscreen)
{
    UFPSSaveGame* SaveGame = LoadSettings();
    if (!SaveGame)
        SaveGame = NewObject<UFPSSaveGame>();

    SaveGame->MasterVolume = Master;
    SaveGame->BackgroundVolume = Background;
    SaveGame->Resolution = Res;
    SaveGame->bFullscreen = bInFullscreen;

    UGameplayStatics::SaveGameToSlot(SaveGame, SaveSlotName, UserIndex);
}

// 加载玩家名称，不存在则返回默认值 "Player"
// 流程：调用 LoadSettings 获取存档 → 如果 PlayerName 非空则返回 → 否则返回 "Player"
FString UFPSSaveGame::LoadPlayerName()
{
    UFPSSaveGame* Save = LoadSettings();
    if (Save && !Save->PlayerName.IsEmpty())
        return Save->PlayerName;
    return TEXT("Player");
}

// 保存玩家名称，先加载已有存档以保留其他设置，再写入并保存
// 流程：调用 LoadSettings 加载已有存档 → 更新 PlayerName → SaveGameToSlot 保存
void UFPSSaveGame::SavePlayerName(const FString& Name)
{
    UFPSSaveGame* Save = LoadSettings();
    if (!Save)
        Save = NewObject<UFPSSaveGame>();

    Save->PlayerName = Name;
    UGameplayStatics::SaveGameToSlot(Save, SaveSlotName, UserIndex);
}

// 将准星配置结构体拆解为独立属性存入独立存档槽
// 流程：新建 UFPSSaveGame 对象 → 将 FCrosshairConfig 各字段拷贝到 UPROPERTY → SaveGameToSlot 到 CrosshairSlotName
void UFPSSaveGame::SaveCrosshairConfig(const FCrosshairConfig& Config)
{
    UFPSSaveGame* Save = NewObject<UFPSSaveGame>();

    Save->CrosshairShape = static_cast<int32>(Config.Shape);
    Save->CrosshairShowCenterDot = Config.bShowCenterDot;
    Save->CrosshairCenterDotRadius = Config.CenterDotRadius;
    Save->CrosshairCenterDotColor = Config.CenterDotColor;
    Save->CrosshairCircleRadius = Config.CircleRadius;
    Save->CrosshairCircleThickness = Config.CircleThickness;
    Save->CrosshairCircleFilled = Config.bCircleFilled;
    Save->CrosshairBarLength = Config.BarLength;
    Save->CrosshairBarThickness = Config.BarThickness;
    Save->CrosshairBarGap = Config.BarGap;
    Save->CrosshairColor = Config.CrosshairColor;
    Save->CrosshairOverallScale = Config.OverallScale;

    UGameplayStatics::SaveGameToSlot(Save, CrosshairSlotName, UserIndex);
}

// 从独立存档槽加载准星配置，不存在则返回默认构造的 FCrosshairConfig
// 流程：检查 CrosshairSlotName 存档是否存在 → 不存在返回默认值 → 存在则 LoadGameFromSlot 并 Cast → 将 UPROPERTY 各字段拷贝回 FCrosshairConfig → 返回
FCrosshairConfig UFPSSaveGame::LoadCrosshairConfig()
{
    FCrosshairConfig Default;
    if (!UGameplayStatics::DoesSaveGameExist(CrosshairSlotName, UserIndex))
        return Default;

    UFPSSaveGame* Save = Cast<UFPSSaveGame>(
        UGameplayStatics::LoadGameFromSlot(CrosshairSlotName, UserIndex));
    if (!Save) return Default;

    Default.Shape = static_cast<ECrosshairShape>(Save->CrosshairShape);
    Default.bShowCenterDot = Save->CrosshairShowCenterDot;
    Default.CenterDotRadius = Save->CrosshairCenterDotRadius;
    Default.CenterDotColor = Save->CrosshairCenterDotColor;
    Default.CircleRadius = Save->CrosshairCircleRadius;
    Default.CircleThickness = Save->CrosshairCircleThickness;
    Default.bCircleFilled = Save->CrosshairCircleFilled;
    Default.BarLength = Save->CrosshairBarLength;
    Default.BarThickness = Save->CrosshairBarThickness;
    Default.BarGap = Save->CrosshairBarGap;
    Default.CrosshairColor = Save->CrosshairColor;
    Default.OverallScale = Save->CrosshairOverallScale;

    return Default;
}
