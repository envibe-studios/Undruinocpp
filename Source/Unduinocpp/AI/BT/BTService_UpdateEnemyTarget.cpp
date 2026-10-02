#include "AI/BT/BTService_UpdateEnemyTarget.h"
#include "AI/EnemyAIController.h"

UBTService_UpdateEnemyTarget::UBTService_UpdateEnemyTarget()
{
	NodeName = TEXT("Update Enemy Target");
	Interval = 0.25f;
	RandomDeviation = 0.05f;
}

void UBTService_UpdateEnemyTarget::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);
	if (AEnemyAIController* AIC = Cast<AEnemyAIController>(OwnerComp.GetAIOwner()))
	{
		AIC->UpdateTargetFromPolicy();
	}
}
