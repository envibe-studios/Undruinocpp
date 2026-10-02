#include "AI/BT/BTTask_EnemyIdle.h"
#include "AI/EnemyAIController.h"
#include "AI/EnemyPawn.h"
#include "AI/EnemyMovementComponent.h"
#include "AI/EnemyTypes.h"

UBTTask_EnemyIdle::UBTTask_EnemyIdle()
{
	NodeName = TEXT("Enemy Idle");
	bNotifyTick = true;
}

EBTNodeResult::Type UBTTask_EnemyIdle::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	float& TimeLeft = *reinterpret_cast<float*>(NodeMemory);
	TimeLeft = IdleDuration;

	if (AEnemyAIController* AIC = Cast<AEnemyAIController>(OwnerComp.GetAIOwner()))
	{
		AIC->SetCombatState(EEnemyCombatState::Idle);
		if (AEnemyPawn* Enemy = Cast<AEnemyPawn>(AIC->GetPawn()))
		{
			if (Enemy->MovementComponent)
			{
				Enemy->MovementComponent->StopMovement();
			}
		}
	}

	if (IdleDuration <= 0.0f)
	{
		return EBTNodeResult::Succeeded;
	}
	return EBTNodeResult::InProgress;
}

void UBTTask_EnemyIdle::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	float& TimeLeft = *reinterpret_cast<float*>(NodeMemory);
	TimeLeft -= DeltaSeconds;
	if (TimeLeft <= 0.0f)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}
