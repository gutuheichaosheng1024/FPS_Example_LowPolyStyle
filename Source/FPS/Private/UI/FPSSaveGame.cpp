#include "UI/FPSSaveGame.h"
#include "Kismet/GameplayStatics.h"

const FString UFPSSaveGame::SaveSlotName = TEXT("FPS_Settings");
const int32 UFPSSaveGame::UserIndex = 0;

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
