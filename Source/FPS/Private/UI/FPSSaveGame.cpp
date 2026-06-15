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
    UFPSSaveGame* SaveGame = NewObject<UFPSSaveGame>();
    SaveGame->MasterVolume = Master;
    SaveGame->BackgroundVolume = Background;
    SaveGame->Resolution = Res;
    SaveGame->bFullscreen = bInFullscreen;

    UGameplayStatics::SaveGameToSlot(SaveGame, SaveSlotName, UserIndex);
}
