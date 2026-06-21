#include "AI/BTTask_ClearBlackboardKey.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_ClearBlackboardKey::UBTTask_ClearBlackboardKey()
{
    NodeName = TEXT("Clear Blackboard Key");
}

// 清除指定黑板键：验证黑板组件和键名有效性 → 调用ClearValue → 返回成功
// 流程：获取BlackboardComponent → 检查KeyToClear是否有效 → BB->ClearValue → EBTNodeResult::Succeeded
EBTNodeResult::Type UBTTask_ClearBlackboardKey::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
    if (BB && KeyToClear.SelectedKeyName != NAME_None)
    {
        BB->ClearValue(KeyToClear.SelectedKeyName);
    }
    return EBTNodeResult::Succeeded;
}
