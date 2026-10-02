#include "AI/StationaryMovementMode.h"
#include "AI/EnemyPawn.h"

void UStationaryMovementMode::TickMovement(float DeltaTime)
{
	if (!OwnerPawn || !bFaceCombatFocus)
	{
		return;
	}

	AActor* Focus = CombatFocusActor.Get();
	if (!IsValid(Focus))
	{
		return;
	}

	const FVector ToFocus = Focus->GetActorLocation() - OwnerPawn->GetActorLocation();
	FaceDirection(DeltaTime, ToFocus);
}

bool UStationaryMovementMode::MoveToLocation(const FVector& WorldLocation, float AcceptanceRadiusOverride)
{
	// Stationary emplacements ignore move goals but still report "arrived"
	// so BT/auto-pursue callers do not spin waiting for arrival.
	MoveGoal = WorldLocation;
	bHasMoveGoal = true;
	return true;
}
