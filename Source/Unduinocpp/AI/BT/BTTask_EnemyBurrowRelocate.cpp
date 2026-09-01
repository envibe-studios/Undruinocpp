#include "AI/BT/BTTask_EnemyBurrowRelocate.h"
#include "AI/EnemyAIController.h"
#include "AI/EnemyPawn.h"
#include "AI/EnemyMovementComponent.h"
#include "AI/BurrowingMovementMode.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AI/EnemyTypes.h"

UBTTask_EnemyBurrowRelocate::UBTTask_EnemyBurrowRelocate()
{
	NodeName = TEXT("Enemy Burrow Relocate");
	bNotifyTick = true;
	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_EnemyBurrowRelocate, TargetActorKey), AActor::StaticClass());
}

EBTNodeResult::Type UBTTask_EnemyBurrowRelocate::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AEnemyAIController* AIC = Cast<AEnemyAIController>(OwnerComp.GetAIOwner());
	AEnemyPawn* Enemy = AIC ? Cast<AEnemyPawn>(AIC->GetPawn()) : nullptr;
	if (!Enemy || !Enemy->MovementComponent)
	{
		return EBTNodeResult::Failed;
	}

	UBurrowingMovementMode* Burrow = Cast<UBurrowingMovementMode>(Enemy->MovementComponent->GetMovementMode());
	if (!Burrow)
	{
		return EBTNodeResult::Failed;
	}

	FVector Emerge = Enemy->GetActorLocation();
	if (UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent())
	{
		const FName Key = TargetActorKey.SelectedKeyName.IsNone() ? FEnemyBlackboardKeys::TargetActor : TargetActorKey.SelectedKeyName;
		if (AActor* Target = Cast<AActor>(BB->GetValueAsObject(Key)))
		{
			Emerge = Target->GetActorLocation();
		}
		else
		{
			Emerge = BB->GetValueAsVector(FEnemyBlackboardKeys::LastKnownLocation);
		}
		BB->SetValueAsEnum(FEnemyBlackboardKeys::MovementState, static_cast<uint8>(EEnemyMovementState::Burrowed));
	}

	if (!Burrow->StartBurrowRelocate(Emerge))
	{
		return EBTNodeResult::Failed;
	}

	return EBTNodeResult::InProgress;
}

void UBTTask_EnemyBurrowRelocate::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AEnemyAIController* AIC = Cast<AEnemyAIController>(OwnerComp.GetAIOwner());
	AEnemyPawn* Enemy = AIC ? Cast<AEnemyPawn>(AIC->GetPawn()) : nullptr;
	UBurrowingMovementMode* Burrow = Enemy && Enemy->MovementComponent
		? Cast<UBurrowingMovementMode>(Enemy->MovementComponent->GetMovementMode())
		: nullptr;

	if (!Burrow)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	if (!Burrow->IsBurrowInProgress())
	{
		if (UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent())
		{
			BB->SetValueAsEnum(FEnemyBlackboardKeys::MovementState, static_cast<uint8>(EEnemyMovementState::Normal));
		}
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}
