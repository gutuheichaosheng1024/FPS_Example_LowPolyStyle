#include "UI/FPSGameEndWidget.h"
#include "FPSGameInstance.h"
#include "Components/RichTextBlock.h"
#include "Components/Button.h"

void UFPSGameEndWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (BackToLobbyButton)
	{
		BackToLobbyButton->OnClicked.AddDynamic(this, &UFPSGameEndWidget::OnBackToLobbyClicked);
	}
}

void UFPSGameEndWidget::UpdateScoreboard(
	const FString& Top1Name, float Top1Score, int32 Top1Kills,
	const FString& Top2Name, float Top2Score, int32 Top2Kills,
	const FString& Top3Name, float Top3Score, int32 Top3Kills)
{
	if (!ScoreboardText) return;

	// 构建三段 RichText，制表符缩进：Top1=0, Top2=1, Top3=2
	const FString Line1 = BuildRow(FString::Printf(TEXT("%s1"), *StyleRowPrefix.ToString()), Top1Name, Top1Score, Top1Kills);
	const FString Line2 = TEXT("\t") + BuildRow(FString::Printf(TEXT("%s2"), *StyleRowPrefix.ToString()), Top2Name, Top2Score, Top2Kills);
	const FString Line3 = TEXT("\t\t") + BuildRow(FString::Printf(TEXT("%s3"), *StyleRowPrefix.ToString()), Top3Name, Top3Score, Top3Kills);

	const FString FullText = Line1 + TEXT("\n") + Line2 + TEXT("\n") + Line3;
	ScoreboardText->SetText(FText::FromString(FullText));
}

FString UFPSGameEndWidget::BuildRow(const FString& StyleTag, const FString& Name, float Score, int32 Kills) const
{
	// 名称部分带 RichText 样式标签，其余为普通文本
	FString StyledName = FString::Printf(TEXT("<%s>%s</>"), *StyleTag, *Name);

	FString Row = RowFormat;
	Row.ReplaceInline(TEXT("{Name}"), *StyledName);
	Row.ReplaceInline(TEXT("{Score}"), *FString::SanitizeFloat(Score, 0));
	Row.ReplaceInline(TEXT("{Kills}"), *FString::FromInt(Kills));

	return Row;
}

void UFPSGameEndWidget::OnBackToLobbyClicked()
{
	if (UFPSGameInstance* GI = Cast<UFPSGameInstance>(GetGameInstance()))
	{
		GI->ReturnToMainMenu();
	}
}
