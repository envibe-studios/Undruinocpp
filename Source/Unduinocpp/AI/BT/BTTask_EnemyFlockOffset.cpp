#include "AI/BT/BTTask_EnemyFlockOffset.h"
#include "AI/EnemyAIController.h"
#include "AI/EnemyPawn.h"
#include "AI/EnemyMovementComponent.h"
#include "AI/EnemyDefinition.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AI/EnemyTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

UBTTask_EnemyFlockOffset::UBTTask_EnemyFlockOffset()
{
	NodeName = TEXT("Enemy Flock Offset");
}

EBTNodeResult::Type UBTTask_EnemyFlockOffset::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AEnemyAIController* AIC = Cast<AEnemyAIController>(OwnerComp.GetAIOwner());
	AEnemyPawn* Enemy = AIC ? Cast<AEnemyPawn>(AIC->GetPawn()) : nullptr;
	if (!Enemy || !Enemy->MovementComponent)
	{
		return EBTNodeResult::Failed;
	}

	UWorld* World = Enemy->GetWorld();
	if (!World)
	{
		return EBTNodeResult::Failed;
	}

	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(World, AEnemyPawn::StaticClass(), Found);

	const FName MyId = Enemy->GetEnemyDefinition() ? Enemy->GetEnemyDefinition()->EnemyId : NAME_None;
	FVector Separation = FVector::ZeroVector;
	FVector Cohesion = FVector::ZeroVector;
	int32 Count = 0;
	const FVector Origin = Enemy->GetActorLocation();
	const float SeparationDist = Enemy->MovementComponent->MovementParams.FlockSeparation;

	for (AActor* Actor : Found)
	{
		AEnemyPawn* Other = Cast<AEnemyPawn>(Actor);
		if (!Other || Other == Enemy)
		{
			continue;
		}
		if (MyId != NAME_None && Other->GetEnemyDefinition() && Other->GetEnemyDefinition()->EnemyId != MyId)
		{
			continue;
		}

		const float Dist = FVector::Dist(Origin, Other->GetActorLocation());
		if (Dist > AllySearchRadius || Dist < KINDA_SMALL_NUMBER)
		{
			continue;
		}

		Cohesion += Other->GetActorLocation();
		++Count;

		if (Dist < SeparationDist)
		{
			Separation += (Origin - Other->GetActorLocation()).GetSafeNormal() * (SeparationDist - Dist);
		}
	}

	FVector Offset = Separation;
	if (Count > 0)
	{
		Cohesion /= static_cast<float>(Count);
		Offset += (Cohesion - Origin) * 0.15f;
	}

	Enemy->MovementComponent->SetFlockOffset(Offset);

	if (UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent())
	{
		BB->SetValueAsInt(FEnemyBlackboardKeys::NearbyAllyCount, Count);
	}

	return EBTNodeResult::Succeeded;
}
