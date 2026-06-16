#include "UI/FPSRespawnWidget.h"
#include "FPSPlayerController.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"

void UFPSRespawnWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (RespawnButton)
	{
		RespawnButton->OnClicked.AddDynamic(this, &UFPSRespawnWidget::OnRespawnClicked);
	}
}

void UFPSRespawnWidget::SetKillerName(const FString& Name)
{
	if (KillerNameText)
	{
		FString Text = KillerNameFormat.Replace(TEXT("{0}"), *Name);
		KillerNameText->SetText(FText::FromString(Text));
	}
}

void UFPSRespawnWidget::OnRespawnClicked()
{
	if (AFPSPlayerController* PC = Cast<AFPSPlayerController>(GetOwningPlayer()))
	{
		PC->Server_RequestRespawn();
	}

	RemoveFromParent();
}
