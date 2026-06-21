#include "UI/FPSGameEndWidget.h"
#include "FPSGameInstance.h"
#include "Components/RichTextBlock.h"
#include "Components/Button.h"

// 初始化游戏结束界面，绑定返回大厅按钮
// 流程：调用 Super::NativeConstruct → 绑定 BackToLobbyButton 的 OnClicked 到 OnBackToLobbyClicked
void UFPSGameEndWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (BackToLobbyButton)
	{
		BackToLobbyButton->OnClicked.AddDynamic(this, &UFPSGameEndWidget::OnBackToLobbyClicked);
	}
}

// 更新计分板显示前三名玩家信息
// 流程：检查 ScoreboardText 有效性 → 构建三段 RichText（Top1 无缩进、Top2 一个制表符、Top3 两个制表符） → 拼接换行符 → SetText 到 ScoreboardText
void UFPSGameEndWidget::UpdateScoreboard(
	const FString& Top1Name, float Top1Score, int32 Top1Kills,
	const FString& Top2Name, float Top2Score, int32 Top2Kills,
	const FString& Top3Name, float Top3Score, int32 Top3Kills)
{
	if (!ScoreboardText) return;

	const FString Line1 = BuildRow(FString::Printf(TEXT("%s1"), *StyleRowPrefix.ToString()), Top1Name, Top1Score, Top1Kills);
	const FString Line2 = TEXT("\t") + BuildRow(FString::Printf(TEXT("%s2"), *StyleRowPrefix.ToString()), Top2Name, Top2Score, Top2Kills);
	const FString Line3 = TEXT("\t\t") + BuildRow(FString::Printf(TEXT("%s3"), *StyleRowPrefix.ToString()), Top3Name, Top3Score, Top3Kills);

	const FString FullText = Line1 + TEXT("\n") + Line2 + TEXT("\n") + Line3;
	ScoreboardText->SetText(FText::FromString(FullText));
}

// 构建单行 RichText 字符串，替换模板占位符并包裹样式标签
// 流程：复制 RowFormat 模板 → 替换 {Name}/{Score}/{Kills} 占位符 → 用样式标签包裹整行文本
FString UFPSGameEndWidget::BuildRow(const FString& StyleTag, const FString& Name, float Score, int32 Kills) const
{
	FString Row = RowFormat;
	Row.ReplaceInline(TEXT("{Name}"), *Name);
	Row.ReplaceInline(TEXT("{Score}"), *FString::SanitizeFloat(Score, 0));
	Row.ReplaceInline(TEXT("{Kills}"), *FString::FromInt(Kills));

	return FString::Printf(TEXT("<%s>%s</>"), *StyleTag, *Row);
}

// 返回大厅按钮回调
// 流程：Cast 到 UFPSGameInstance → 调用 ReturnToMainMenu 返回主菜单
void UFPSGameEndWidget::OnBackToLobbyClicked()
{
	if (UFPSGameInstance* GI = Cast<UFPSGameInstance>(GetGameInstance()))
	{
		GI->ReturnToMainMenu();
	}
}
