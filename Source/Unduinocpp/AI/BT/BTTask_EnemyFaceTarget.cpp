#include "AI/BT/BTTask_EnemyFaceTarget.h"
#include "AI/EnemyAIController.h"
#include "AI/EnemyPawn.h"
#include "AI/EnemyMovementComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AI/EnemyTypes.h"

UBTTask_EnemyFaceTarget::UBTTask_EnemyFaceTarget()
{
	NodeName = TEXT("Enemy Face Target");
	bNotifyTick = true;
	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_EnemyFaceTarget, TargetActorKey), AActor::StaticClass());
}

EBTNodeResult::Type UBTTask_EnemyFaceTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	return EBTNodeResult::InProgress;
}

void UBTTask_EnemyFaceTarget::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AEnemyAIController* AIC = Cast<AEnemyAIController>(OwnerComp.GetAIOwner());
	AEnemyPawn* Enemy = AIC ? Cast<AEnemyPawn>(AIC->GetPawn()) : nullptr;
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!Enemy || !BB)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	const FName Key = TargetActorKey.SelectedKeyName.IsNone() ? FEnemyBlackboardKeys::TargetActor : TargetActorKey.SelectedKeyName;
	AActor* Target = Cast<AActor>(BB->GetValueAsObject(Key));
	if (!Target)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	FVector ToTarget = Target->GetActorLocation() - Enemy->GetActorLocation();
	ToTarget.Z = 0.0f;
	if (ToTarget.IsNearlyZero())
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	const FRotator Desired = ToTarget.GetSafeNormal().Rotation();
	const FRotator Current = Enemy->GetActorRotation();
	const float TurnRate = Enemy->MovementComponent ? Enemy->MovementComponent->MovementParams.TurnRateDegPerSec : 180.0f;
	const FRotator NewRot = FMath::RInterpConstantTo(Current, Desired, DeltaSeconds, TurnRate);
	Enemy->SetActorRotation(NewRot);

	const float Angle = FMath::Abs(FRotator::NormalizeAxis(NewRot.Yaw - Desired.Yaw));
	if (Angle <= AngleToleranceDeg)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}
