#include "AI/CrawlingMovementMode.h"
#include "AI/EnemyPawn.h"
#include "NavigationSystem.h"
#include "Engine/World.h"

FVector UCrawlingMovementMode::ProjectGoalToGround(const FVector& Desired) const
{
	if (!OwnerPawn)
	{
		return Desired;
	}

	UWorld* World = OwnerPawn->GetWorld();
	if (!World)
	{
		return Desired;
	}

	if (bProjectToNavMesh)
	{
		if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
		{
			FNavLocation Projected;
			if (NavSys->ProjectPointToNavigation(Desired, Projected))
			{
				return Projected.Location;
			}
		}
	}

	FHitResult Hit;
	const FVector Start = Desired + FVector(0, 0, GroundTraceHeight);
	const FVector End = Desired - FVector(0, 0, GroundTraceHeight * 4.0f);
	FCollisionQueryParams ParamsQuery(SCENE_QUERY_STAT(CrawlGround), false, OwnerPawn);
	if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, ParamsQuery))
	{
		return Hit.ImpactPoint + FVector(0, 0, 5.0f);
	}
	return Desired;
}

bool UCrawlingMovementMode::MoveToLocation(const FVector& WorldLocation, float AcceptanceRadiusOverride)
{
	return Super::MoveToLocation(ProjectGoalToGround(WorldLocation), AcceptanceRadiusOverride);
}

void UCrawlingMovementMode::TickMovement(float DeltaTime)
{
	if (!OwnerPawn || !bHasMoveGoal || MovementState == EEnemyMovementState::Carried || MovementState == EEnemyMovementState::Burrowed)
	{
		return;
	}

	FVector Goal = MoveGoal;
	if (bHasFlockOffset)
	{
		Goal += FlockOffset;
		Goal = ProjectGoalToGround(Goal);
	}

	const FVector Loc = OwnerPawn->GetActorLocation();
	FVector ToGoal = Goal - Loc;
	ToGoal.Z = 0.0f;
	const float Dist = ToGoal.Size();
	if (Dist <= Params.AcceptanceRadius)
	{
		MoveOwnerToward(DeltaTime, FVector::ZeroVector);
		return;
	}

	const FVector DesiredVel = ToGoal.GetSafeNormal() * Params.MaxSpeed;
	MoveOwnerToward(DeltaTime, DesiredVel);

	// Stick to ground
	const FVector Grounded = ProjectGoalToGround(OwnerPawn->GetActorLocation());
	FVector NewLoc = OwnerPawn->GetActorLocation();
	NewLoc.Z = FMath::FInterpTo(NewLoc.Z, Grounded.Z, DeltaTime, 8.0f);
	OwnerPawn->SetActorLocation(NewLoc, true);
}
