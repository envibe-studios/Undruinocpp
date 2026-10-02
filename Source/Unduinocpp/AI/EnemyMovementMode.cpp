#include "AI/EnemyMovementMode.h"
#include "AI/EnemyPawn.h"
#include "AI/EnemyMovementComponent.h"

void UEnemyMovementMode::Initialize(AEnemyPawn* InOwner, UEnemyMovementComponent* InMovement)
{
	OwnerPawn = InOwner;
	MovementComp = InMovement;
	if (MovementComp)
	{
		ApplyParams(MovementComp->MovementParams);
	}
}

void UEnemyMovementMode::Shutdown()
{
	StopMovement();
	OwnerPawn = nullptr;
	MovementComp = nullptr;
}

void UEnemyMovementMode::ApplyParams(const FEnemyMovementParams& InParams)
{
	Params = InParams;
}

void UEnemyMovementMode::TickMovement(float DeltaTime)
{
}

void UEnemyMovementMode::StopMovement()
{
	ClearMoveGoal();
	bHasFlockOffset = false;
	FlockOffset = FVector::ZeroVector;
	CurrentVelocity = FVector::ZeroVector;
}

bool UEnemyMovementMode::MoveToLocation(const FVector& WorldLocation, float AcceptanceRadiusOverride)
{
	MoveGoal = WorldLocation;
	bHasMoveGoal = true;
	const float Radius = AcceptanceRadiusOverride >= 0.0f ? AcceptanceRadiusOverride : Params.AcceptanceRadius;
	if (OwnerPawn)
	{
		const float DistSq = FVector::DistSquared(OwnerPawn->GetActorLocation(), WorldLocation);
		return DistSq <= FMath::Square(Radius);
	}
	return false;
}

void UEnemyMovementMode::SetFlockOffset(const FVector& WorldOffset)
{
	FlockOffset = WorldOffset;
	bHasFlockOffset = !WorldOffset.IsNearlyZero();
}

void UEnemyMovementMode::ClearMoveGoal()
{
	bHasMoveGoal = false;
	MoveGoal = FVector::ZeroVector;
	CombatFocusActor.Reset();
}

bool UEnemyMovementMode::AttachPassenger(AActor* Passenger)
{
	if (!Passenger || !OwnerPawn)
	{
		return false;
	}
	PassengerActor = Passenger;
	Passenger->AttachToActor(OwnerPawn, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	MovementState = EEnemyMovementState::Carrying;
	return true;
}

bool UEnemyMovementMode::DropPassenger()
{
	if (AActor* Passenger = PassengerActor.Get())
	{
		Passenger->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	}
	PassengerActor.Reset();
	if (MovementState == EEnemyMovementState::Carrying)
	{
		MovementState = EEnemyMovementState::Normal;
	}
	return true;
}

bool UEnemyMovementMode::BeginBurrow()
{
	return false;
}

bool UEnemyMovementMode::EndBurrowAt(const FVector& EmergeLocation)
{
	return false;
}

void UEnemyMovementMode::FaceDirection(float DeltaTime, const FVector& WorldDirection)
{
	if (!OwnerPawn || WorldDirection.IsNearlyZero())
	{
		return;
	}

	const FVector FlatDir = FVector(WorldDirection.X, WorldDirection.Y, 0.0f).GetSafeNormal();
	if (FlatDir.IsNearlyZero())
	{
		return;
	}

	const FRotator Current = OwnerPawn->GetActorRotation();
	const FRotator Desired = FlatDir.Rotation();
	const FRotator NewRot = FMath::RInterpConstantTo(Current, Desired, DeltaTime, Params.TurnRateDegPerSec);
	OwnerPawn->SetActorRotation(NewRot);
}

void UEnemyMovementMode::MoveOwnerToward(float DeltaTime, const FVector& DesiredVelocity, bool bAutoFace)
{
	if (!OwnerPawn || DeltaTime <= 0.0f)
	{
		return;
	}

	CurrentVelocity = FMath::VInterpConstantTo(CurrentVelocity, DesiredVelocity, DeltaTime, Params.Acceleration);

	const FVector NewLoc = OwnerPawn->GetActorLocation() + CurrentVelocity * DeltaTime;

	// Kinematic AI: prefer swept move, but don't stall forever if already overlapping.
	FHitResult Hit;
	if (!OwnerPawn->SetActorLocation(NewLoc, true, &Hit))
	{
		OwnerPawn->SetActorLocation(NewLoc, false);
	}

	if (bAutoFace && !DesiredVelocity.IsNearlyZero())
	{
		FaceDirection(DeltaTime, DesiredVelocity);
	}
}
