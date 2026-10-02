#include "AI/BT/BTTask_EnemyMoveTo.h"
#include "AI/EnemyAIController.h"
#include "AI/EnemyPawn.h"
#include "AI/EnemyMovementComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AI/EnemyTypes.h"

UBTTask_EnemyMoveTo::UBTTask_EnemyMoveTo()
{
	NodeName = TEXT("Enemy Move To");
	bNotifyTick = true;
	MoveGoalKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_EnemyMoveTo, MoveGoalKey));
	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_EnemyMoveTo, TargetActorKey), AActor::StaticClass());
}

EBTNodeResult::Type UBTTask_EnemyMoveTo::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AEnemyAIController* AIC = Cast<AEnemyAIController>(OwnerComp.GetAIOwner());
	AEnemyPawn* Enemy = AIC ? Cast<AEnemyPawn>(AIC->GetPawn()) : nullptr;
	if (!Enemy || !Enemy->MovementComponent)
	{
		return EBTNodeResult::Failed;
	}

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	FVector Goal = FVector::ZeroVector;
	bool bHaveGoal = false;

	if (BB)
	{
		if (AActor* Target = Cast<AActor>(BB->GetValueAsObject(TargetActorKey.SelectedKeyName.IsNone() ? FEnemyBlackboardKeys::TargetActor : TargetActorKey.SelectedKeyName)))
		{
			Goal = Target->GetActorLocation();
			bHaveGoal = true;
		}
		else
		{
			const FName GoalKey = MoveGoalKey.SelectedKeyName.IsNone() ? FEnemyBlackboardKeys::MoveGoal : MoveGoalKey.SelectedKeyName;
			Goal = BB->GetValueAsVector(GoalKey);
			bHaveGoal = !Goal.IsNearlyZero();
			if (!bHaveGoal)
			{
				Goal = BB->GetValueAsVector(FEnemyBlackboardKeys::LastKnownLocation);
				bHaveGoal = !Goal.IsNearlyZero();
			}
		}
	}

	if (!bHaveGoal)
	{
		return EBTNodeResult::Failed;
	}

	const bool bArrived = Enemy->MovementComponent->MoveToLocation(Goal, AcceptanceRadius);
	return bArrived ? EBTNodeResult::Succeeded : EBTNodeResult::InProgress;
}

void UBTTask_EnemyMoveTo::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AEnemyAIController* AIC = Cast<AEnemyAIController>(OwnerComp.GetAIOwner());
	AEnemyPawn* Enemy = AIC ? Cast<AEnemyPawn>(AIC->GetPawn()) : nullptr;
	if (!Enemy || !Enemy->MovementComponent)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	FVector Goal = FVector::ZeroVector;

	if (BB && bTrackMovingTarget)
	{
		const FName TargetKey = TargetActorKey.SelectedKeyName.IsNone() ? FEnemyBlackboardKeys::TargetActor : TargetActorKey.SelectedKeyName;
		if (AActor* Target = Cast<AActor>(BB->GetValueAsObject(TargetKey)))
		{
			Goal = Target->GetActorLocation();
		}
	}

	if (Goal.IsNearlyZero() && BB)
	{
		const FName GoalKey = MoveGoalKey.SelectedKeyName.IsNone() ? FEnemyBlackboardKeys::MoveGoal : MoveGoalKey.SelectedKeyName;
		Goal = BB->GetValueAsVector(GoalKey);
		if (Goal.IsNearlyZero())
		{
			Goal = BB->GetValueAsVector(FEnemyBlackboardKeys::LastKnownLocation);
		}
	}

	if (Goal.IsNearlyZero())
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	if (Enemy->MovementComponent->MoveToLocation(Goal, AcceptanceRadius))
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}
