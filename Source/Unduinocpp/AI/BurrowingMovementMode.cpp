#include "AI/BurrowingMovementMode.h"
#include "AI/EnemyPawn.h"
#include "Components/PrimitiveComponent.h"

bool UBurrowingMovementMode::BeginBurrow()
{
	if (!OwnerPawn || bBurrowInProgress)
	{
		return false;
	}

	bBurrowInProgress = true;
	bHiddenUnderground = true;
	BurrowElapsed = 0.0f;
	MovementState = EEnemyMovementState::Burrowed;

	OwnerPawn->SetActorHiddenInGame(true);
	OwnerPawn->SetActorEnableCollision(false);

	return true;
}

bool UBurrowingMovementMode::EndBurrowAt(const FVector& InEmergeLocation)
{
	if (!OwnerPawn)
	{
		return false;
	}

	EmergeLocation = InEmergeLocation;
	OwnerPawn->SetActorLocation(EmergeLocation, false);
	OwnerPawn->SetActorHiddenInGame(false);
	OwnerPawn->SetActorEnableCollision(true);

	bHiddenUnderground = false;
	bBurrowInProgress = false;
	MovementState = EEnemyMovementState::Normal;
	ClearMoveGoal();
	return true;
}

bool UBurrowingMovementMode::StartBurrowRelocate(const FVector& PreferredEmergeLocation)
{
	EmergeLocation = PreferredEmergeLocation;
	if (Params.BurrowRelocateRadius > 0.0f && OwnerPawn)
	{
		const FVector2D Offset = FMath::RandPointInCircle(Params.BurrowRelocateRadius);
		EmergeLocation = PreferredEmergeLocation + FVector(Offset.X, Offset.Y, 0.0f);
	}
	return BeginBurrow();
}

void UBurrowingMovementMode::TickMovement(float DeltaTime)
{
	if (!OwnerPawn)
	{
		return;
	}

	if (bBurrowInProgress)
	{
		BurrowElapsed += DeltaTime;
		if (BurrowElapsed >= Params.BurrowDuration)
		{
			EndBurrowAt(EmergeLocation);
		}
		return;
	}

	if (!bHasMoveGoal || MovementState == EEnemyMovementState::Carried)
	{
		return;
	}

	FVector Goal = MoveGoal;
	if (bHasFlockOffset)
	{
		Goal += FlockOffset;
	}

	const FVector Loc = OwnerPawn->GetActorLocation();
	FVector ToGoal = Goal - Loc;
	ToGoal.Z = 0.0f;
	if (ToGoal.Size() <= Params.AcceptanceRadius)
	{
		MoveOwnerToward(DeltaTime, FVector::ZeroVector);
		return;
	}

	MoveOwnerToward(DeltaTime, ToGoal.GetSafeNormal() * Params.MaxSpeed);
}
