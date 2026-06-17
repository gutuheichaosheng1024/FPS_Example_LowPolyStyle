#include "UI/FPSSaveGame.h"
#include "Kismet/GameplayStatics.h"

const FString UFPSSaveGame::SaveSlotName = TEXT("FPS_Settings");
const int32 UFPSSaveGame::UserIndex = 0;
static const FString CrosshairSlotName = TEXT("FPS_Crosshair");

UFPSSaveGame* UFPSSaveGame::LoadSettings()
{
    if (UGameplayStatics::DoesSaveGameExist(SaveSlotName, UserIndex))
    {
        return Cast<UFPSSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlotName, UserIndex));
    }

    // 返回默认值
    return NewObject<UFPSSaveGame>();
}

void UFPSSaveGame::SaveSettings(float Master, float Background, const FString& Res, bool bInFullscreen)
{
    // 加载已有存档以保留 PlayerName
    UFPSSaveGame* SaveGame = LoadSettings();
    if (!SaveGame)
        SaveGame = NewObject<UFPSSaveGame>();

    SaveGame->MasterVolume = Master;
    SaveGame->BackgroundVolume = Background;
    SaveGame->Resolution = Res;
    SaveGame->bFullscreen = bInFullscreen;

    UGameplayStatics::SaveGameToSlot(SaveGame, SaveSlotName, UserIndex);
}

FString UFPSSaveGame::LoadPlayerName()
{
    UFPSSaveGame* Save = LoadSettings();
    if (Save && !Save->PlayerName.IsEmpty())
        return Save->PlayerName;
    return TEXT("Player");
}

void UFPSSaveGame::SavePlayerName(const FString& Name)
{
    // 加载已有存档以保留其他设置
    UFPSSaveGame* Save = LoadSettings();
    if (!Save)
        Save = NewObject<UFPSSaveGame>();

    Save->PlayerName = Name;
    UGameplayStatics::SaveGameToSlot(Save, SaveSlotName, UserIndex);
}

// ======================================================================
// 准星配置读写
// ======================================================================

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
