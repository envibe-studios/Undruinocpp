#include "AI/FloatingMovementMode.h"
#include "AI/EnemyPawn.h"
#include "Engine/World.h"

void UFloatingMovementMode::TickMovement(float DeltaTime)
{
	if (!OwnerPawn || MovementState == EEnemyMovementState::Carried)
	{
		return;
	}

	FVector Loc = OwnerPawn->GetActorLocation();
	FVector DesiredVel = FVector::ZeroVector;
	const float Time = OwnerPawn->GetWorld() ? OwnerPawn->GetWorld()->GetTimeSeconds() : 0.0f;
	const FVector Sway(
		FMath::Sin(Time * 0.6f + Loc.X * 0.001f) * 180.0f,
		FMath::Cos(Time * 0.45f + Loc.Y * 0.001f) * 180.0f,
		FMath::Sin(Time * 0.9f) * 90.0f);

	if (bHasMoveGoal)
	{
		FVector Goal = MoveGoal;
		if (bHasFlockOffset)
		{
			Goal += FlockOffset;
		}

		// Drift toward a soft orbit ring instead of a beeline into the target.
		const FVector FlatAway = (Loc - MoveGoal).GetSafeNormal2D();
		const float SoftOrbit = 900.0f;
		Goal = MoveGoal + (FlatAway.IsNearlyZero() ? FVector::ForwardVector : FlatAway) * SoftOrbit;
		Goal.Z = MoveGoal.Z + Params.PreferredAltitude;
		Goal += Sway;

		const FVector ToGoal = Goal - Loc;
		if (ToGoal.Size() > Params.AcceptanceRadius)
		{
			DesiredVel = ToGoal.GetSafeNormal() * Params.MaxSpeed;
		}
	}
	else
	{
		// Idle bob toward preferred altitude above home.
		const float ZError = (Params.PreferredAltitude > 0.0f)
			? (OwnerPawn->GetHomeLocation().Z + Params.PreferredAltitude) - Loc.Z
			: 0.0f;
		DesiredVel = Sway * 0.35f;
		DesiredVel.Z += ZError * HoverDamping;
	}

	// Soften vertical vs horizontal.
	DesiredVel.Z *= 0.6f;
	MoveOwnerToward(DeltaTime, DesiredVel);
}
