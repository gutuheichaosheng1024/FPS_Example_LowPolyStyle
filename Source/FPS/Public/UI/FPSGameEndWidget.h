#pragma once

#include "CoreMinimal.h"
#include "UI/FPSBaseMenuWidget.h"
#include "FPSGameEndWidget.generated.h"

class URichTextBlock;
class UButton;

/**
 * UFPSGameEndWidget — 游戏结束界面，使用 RichTextBlock 展示前三名计分板
 *
 * 职责：接收前三名玩家名称/分数/击杀数 → 按 RowFormat 模板构建 RichText 行 → 使用制表符缩进区分名次 → 通过 RichText StyleSet DataTable 控制名次颜色；提供返回大厅按钮
 * 使用：UFPSBaseMenuWidget、UFPSGameInstance、URichTextBlock、UButton
 */
UCLASS(Abstract)
class FPS_API UFPSGameEndWidget : public UFPSBaseMenuWidget
{
	GENERATED_BODY()

public:
	void UpdateScoreboard(
		const FString& Top1Name, float Top1Score, int32 Top1Kills,
		const FString& Top2Name, float Top2Score, int32 Top2Kills,
		const FString& Top3Name, float Top3Score, int32 Top3Kills);

	UPROPERTY(EditDefaultsOnly, Category = "Scoreboard")
	FString RowFormat = TEXT("{Name}  {Score}分  {Kills}杀");

	UPROPERTY(EditDefaultsOnly, Category = "Scoreboard")
	FName StyleRowPrefix = TEXT("Top");

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	URichTextBlock* ScoreboardText;

	UPROPERTY(meta = (BindWidget))
	UButton* BackToLobbyButton;

private:
	FString BuildRow(const FString& StyleTag, const FString& Name, float Score, int32 Kills) const;

	UFUNCTION()
	void OnBackToLobbyClicked();
};
