#pragma once

#include "CoreMinimal.h"
#include "UI/FPSBaseMenuWidget.h"
#include "FPSGameEndWidget.generated.h"

class URichTextBlock;
class UButton;

/**
 * 游戏结束界面（简化版）
 *
 * 1 个 RichTextBlock 拼接展示前三名，制表符缩进区分名次
 * 通过 RichTextBlock 的 TextStyleSet DataTable 控制前三名名称颜色
 *
 * 蓝图子类需要:
 *   1. Rich Text Block 命名 ScoreboardText
 *   2. Button 命名 BackToLobbyButton
 */
UCLASS(Abstract)
class FPS_API UFPSGameEndWidget : public UFPSBaseMenuWidget
{
	GENERATED_BODY()

public:
	/** 更新计分板前三名 */
	void UpdateScoreboard(
		const FString& Top1Name, float Top1Score, int32 Top1Kills,
		const FString& Top2Name, float Top2Score, int32 Top2Kills,
		const FString& Top3Name, float Top3Score, int32 Top3Kills);

	// ---------- 可配置参数 ----------

	/** 每行格式（{Name}/{Score}/{Kills} 占位） */
	UPROPERTY(EditDefaultsOnly, Category = "Scoreboard")
	FString RowFormat = TEXT("{Name}  {Score}分  {Kills}杀");

	/** RichText 样式行名前缀（DataTable 中行名应为 Top1/Top2/Top3） */
	UPROPERTY(EditDefaultsOnly, Category = "Scoreboard")
	FName StyleRowPrefix = TEXT("Top");

protected:
	virtual void NativeConstruct() override;

	// ---------- BindWidget ----------

	UPROPERTY(meta = (BindWidget))
	URichTextBlock* ScoreboardText;

	UPROPERTY(meta = (BindWidget))
	UButton* BackToLobbyButton;

private:
	/** 构建单行的 RichText 字符串 */
	FString BuildRow(const FString& StyleTag, const FString& Name, float Score, int32 Kills) const;

	UFUNCTION()
	void OnBackToLobbyClicked();
};
