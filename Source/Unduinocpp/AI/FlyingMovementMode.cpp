#include "AI/FlyingMovementMode.h"
#include "AI/EnemyPawn.h"
#include "HoverThrusterComponent.h"
#include "Components/PrimitiveComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Math/UnrealMathUtility.h"

void UFlyingMovementMode::Initialize(AEnemyPawn* InOwner, UEnemyMovementComponent* InMovement)
{
	Super::Initialize(InOwner, InMovement);

	NoisePhaseA = FMath::FRandRange(0.0f, TWO_PI);
	NoisePhaseB = FMath::FRandRange(0.0f, TWO_PI);
	NoisePhaseC = FMath::FRandRange(0.0f, TWO_PI);
	OrbitDirection = FMath::RandBool() ? 1.0f : -1.0f;
	OrbitAngle = FMath::FRandRange(0.0f, TWO_PI);
	CombatPhase = EEnemyFlyingCombatPhase::Approach;
	ResetDiveTimer();
}

void UFlyingMovementMode::SetCombatFocus(AActor* InFocusActor)
{
	Super::SetCombatFocus(InFocusActor);
	FocusActor = InFocusActor;
	if (!IsValid(InFocusActor))
	{
		DiveThruster.Reset();
		if (CombatPhase != EEnemyFlyingCombatPhase::Approach)
		{
			EnterPhase(EEnemyFlyingCombatPhase::Approach);
		}
	}
}

void UFlyingMovementMode::ClearMoveGoal()
{
	Super::ClearMoveGoal();
	FocusActor.Reset();
	DiveThruster.Reset();
	EnterPhase(EEnemyFlyingCombatPhase::Approach);
}

void UFlyingMovementMode::StopMovement()
{
	Super::StopMovement();
	FocusActor.Reset();
	DiveThruster.Reset();
	EnterPhase(EEnemyFlyingCombatPhase::Approach);
}

bool UFlyingMovementMode::MoveToLocation(const FVector& WorldLocation, float AcceptanceRadiusOverride)
{
	// Store ship focus location as-is. Altitude / orbit / dive are handled in TickMovement.
	return Super::MoveToLocation(WorldLocation, AcceptanceRadiusOverride);
}

void UFlyingMovementMode::ResetDiveTimer()
{
	DiveCooldownRemaining = FMath::FRandRange(DiveCooldownMin, FMath::Max(DiveCooldownMin, DiveCooldownMax));
}

FVector UFlyingMovementMode::ComputeSwayOffset(float TimeSeconds) const
{
	return FVector(
		FMath::Sin(TimeSeconds * SwayFrequencyA + NoisePhaseA) * SwayAmplitudeXY,
		FMath::Cos(TimeSeconds * SwayFrequencyB + NoisePhaseB) * SwayAmplitudeXY,
		FMath::Sin(TimeSeconds * SwayFrequencyC + NoisePhaseC) * SwayAmplitudeZ);
}

FVector UFlyingMovementMode::GetFocusLocation() const
{
	if (AActor* Focus = FocusActor.Get())
	{
		return Focus->GetActorLocation();
	}
	return MoveGoal;
}

bool UFlyingMovementMode::HasValidFocus() const
{
	return FocusActor.IsValid() || bHasMoveGoal;
}

FVector UFlyingMovementMode::SteerToward(const FVector& DesiredWorldPoint, float Speed) const
{
	if (!OwnerPawn)
	{
		return FVector::ZeroVector;
	}

	const FVector To = DesiredWorldPoint - OwnerPawn->GetActorLocation();
	const float Dist = To.Size();
	if (Dist < 1.0f)
	{
		return FVector::ZeroVector;
	}

	// Ease near the waypoint so orbit doesn't jitter.
	const float Ease = FMath::Clamp(Dist / 400.0f, 0.25f, 1.0f);
	return To.GetSafeNormal() * (Speed * Ease);
}

void UFlyingMovementMode::FaceVelocity(float DeltaTime, const FVector& Velocity)
{
	if (!OwnerPawn || Velocity.SizeSquared() < 100.0f)
	{
		return;
	}

	const FRotator Current = OwnerPawn->GetActorRotation();
	const FRotator Desired = Velocity.GetSafeNormal().Rotation();
	const FRotator NewRot = FMath::RInterpConstantTo(Current, Desired, DeltaTime, Params.TurnRateDegPerSec);
	OwnerPawn->SetActorRotation(NewRot);
}

void UFlyingMovementMode::EnterPhase(EEnemyFlyingCombatPhase NewPhase)
{
	CombatPhase = NewPhase;
	if (NewPhase == EEnemyFlyingCombatPhase::Dive)
	{
		ClosestDiveDistance = TNumericLimits<float>::Max();
		bDiveDamageApplied = false;
	}
}

void UFlyingMovementMode::PickDiveTarget()
{
	DiveThruster.Reset();
	DiveAimPoint = GetFocusLocation();

	AActor* Focus = FocusActor.Get();
	if (!Focus)
	{
		return;
	}

	TArray<UHoverThrusterComponent*> Thrusters;
	Focus->GetComponents<UHoverThrusterComponent>(Thrusters);

	TArray<UHoverThrusterComponent*> Candidates;
	Candidates.Reserve(Thrusters.Num());
	for (UHoverThrusterComponent* Thruster : Thrusters)
	{
		if (Thruster && !Thruster->IsDestroyed())
		{
			Candidates.Add(Thruster);
		}
	}

	if (Candidates.Num() > 0)
	{
		UHoverThrusterComponent* Chosen = Candidates[FMath::RandRange(0, Candidates.Num() - 1)];
		DiveThruster = Chosen;
		DiveAimPoint = Chosen->GetComponentLocation();
	}
	else
	{
		// No thrusters — aim at a random local offset around the ship hull.
		const FVector Offset = Focus->GetActorRotation().RotateVector(
			FVector(FMath::FRandRange(-200.0f, 200.0f), FMath::FRandRange(-400.0f, 400.0f), FMath::FRandRange(-50.0f, 80.0f)));
		DiveAimPoint = Focus->GetActorLocation() + Offset;
	}
}

void UFlyingMovementMode::ApplySoftDiveNudge(AActor* Ship) const
{
	if (!Ship || !OwnerPawn || DiveNudgeSpeed <= 0.0f)
	{
		return;
	}

	UPrimitiveComponent* Phys = Cast<UPrimitiveComponent>(Ship->GetRootComponent());
	if (!Phys || !Phys->IsSimulatingPhysics())
	{
		return;
	}

	FVector Dir = (Ship->GetActorLocation() - OwnerPawn->GetActorLocation());
	Dir.Z *= DiveNudgeVerticalScale;
	if (Dir.IsNearlyZero())
	{
		Dir = OwnerPawn->GetActorForwardVector();
		Dir.Z = 0.0f;
	}
	Dir = Dir.GetSafeNormal();

	// Velocity-change impulse through COM — nudge without thruster-point torque/flip.
	Phys->AddImpulse(Dir * DiveNudgeSpeed, NAME_None, true);
}

void UFlyingMovementMode::TryApplyDiveDamage()
{
	if (bDiveDamageApplied || DiveDamage <= 0.0f)
	{
		return;
	}

	if (ClosestDiveDistance > DiveHitRadius)
	{
		return;
	}

	AActor* Focus = FocusActor.Get();

	if (UHoverThrusterComponent* Thruster = DiveThruster.Get())
	{
		Thruster->ApplyDamage(DiveDamage);
		bDiveDamageApplied = true;
		ApplySoftDiveNudge(Focus);
		return;
	}

	// Fallback: generic actor damage if no thruster was selected.
	if (Focus)
	{
		UGameplayStatics::ApplyDamage(Focus, DiveDamage * 0.5f, OwnerPawn ? OwnerPawn->GetController() : nullptr, OwnerPawn, nullptr);
		bDiveDamageApplied = true;
		ApplySoftDiveNudge(Focus);
	}
}

void UFlyingMovementMode::TickApproach(float DeltaTime)
{
	const float Time = OwnerPawn->GetWorld() ? OwnerPawn->GetWorld()->GetTimeSeconds() : 0.0f;
	const FVector Focus = GetFocusLocation();
	const FVector Sway = ComputeSwayOffset(Time);

	FVector ApproachPoint = Focus;
	ApproachPoint.Z += Params.PreferredAltitude;
	ApproachPoint += Sway;

	// Keep a bit of lead so we don't head-on into the ship.
	const FVector FlatAway = (OwnerPawn->GetActorLocation() - Focus).GetSafeNormal2D();
	if (!FlatAway.IsNearlyZero())
	{
		ApproachPoint += FlatAway * (OrbitRadius * 0.35f);
	}

	const float Dist2D = FVector::Dist2D(OwnerPawn->GetActorLocation(), Focus);
	const FVector DesiredVel = SteerToward(ApproachPoint, Params.MaxSpeed);
	MoveOwnerToward(DeltaTime, DesiredVel, false);
	FaceVelocity(DeltaTime, CurrentVelocity);

	if (Dist2D <= OrbitEnterDistance)
	{
		const FVector Rel = OwnerPawn->GetActorLocation() - Focus;
		OrbitAngle = FMath::Atan2(Rel.Y, Rel.X);
		EnterPhase(EEnemyFlyingCombatPhase::Orbit);
		ResetDiveTimer();
	}
}

void UFlyingMovementMode::TickOrbit(float DeltaTime)
{
	const float Time = OwnerPawn->GetWorld() ? OwnerPawn->GetWorld()->GetTimeSeconds() : 0.0f;
	const FVector Focus = GetFocusLocation();
	const FVector Sway = ComputeSwayOffset(Time);

	const float AngularSpeed = (Params.MaxSpeed / FMath::Max(OrbitRadius, 100.0f)) * OrbitDirection;
	OrbitAngle += AngularSpeed * DeltaTime;

	const float HeightBob = FMath::Sin(Time * SwayFrequencyC + NoisePhaseC) * OrbitHeightVariance;
	FVector OrbitPoint = Focus;
	OrbitPoint.X += FMath::Cos(OrbitAngle) * OrbitRadius;
	OrbitPoint.Y += FMath::Sin(OrbitAngle) * OrbitRadius;
	OrbitPoint.Z += Params.PreferredAltitude + HeightBob;
	OrbitPoint += Sway * 0.45f;

	if (bHasFlockOffset)
	{
		OrbitPoint += FlockOffset;
	}

	const FVector DesiredVel = SteerToward(OrbitPoint, Params.MaxSpeed);
	MoveOwnerToward(DeltaTime, DesiredVel, false);
	FaceVelocity(DeltaTime, CurrentVelocity);

	DiveCooldownRemaining -= DeltaTime;
	if (DiveCooldownRemaining <= 0.0f)
	{
		PickDiveTarget();
		EnterPhase(EEnemyFlyingCombatPhase::Dive);
	}

	// If the ship pulls far away, re-approach.
	if (FVector::Dist2D(OwnerPawn->GetActorLocation(), Focus) > OrbitEnterDistance * 1.75f)
	{
		EnterPhase(EEnemyFlyingCombatPhase::Approach);
	}
}

void UFlyingMovementMode::TickDive(float DeltaTime)
{
	if (UHoverThrusterComponent* Thruster = DiveThruster.Get())
	{
		DiveAimPoint = Thruster->GetComponentLocation();
	}
	else if (AActor* Focus = FocusActor.Get())
	{
		// Keep aiming near ship center if thruster vanished mid-dive.
		DiveAimPoint = FMath::VInterpTo(DiveAimPoint, Focus->GetActorLocation(), DeltaTime, 2.0f);
	}

	const FVector Loc = OwnerPawn->GetActorLocation();
	const float Dist = FVector::Dist(Loc, DiveAimPoint);
	ClosestDiveDistance = FMath::Min(ClosestDiveDistance, Dist);

	const float DiveSpeed = Params.MaxSpeed * DiveSpeedMultiplier;
	const FVector DesiredVel = SteerToward(DiveAimPoint, DiveSpeed);
	MoveOwnerToward(DeltaTime, DesiredVel, false);
	FaceVelocity(DeltaTime, CurrentVelocity);

	const bool bPassedTarget = FVector::DotProduct(CurrentVelocity.GetSafeNormal(), (DiveAimPoint - Loc).GetSafeNormal()) < 0.0f
		&& Dist < DiveHitRadius * 2.5f;
	const bool bCloseEnough = Dist <= DiveHitRadius * 0.65f;

	if (bPassedTarget || bCloseEnough || Dist < 80.0f)
	{
		TryApplyDiveDamage();

		const FVector Focus = GetFocusLocation();
		FVector Away = (Loc - Focus).GetSafeNormal2D();
		if (Away.IsNearlyZero())
		{
			Away = OwnerPawn->GetActorForwardVector().GetSafeNormal2D();
		}
		PullUpPoint = Loc + Away * PullUpDistance;
		PullUpPoint.Z = Focus.Z + Params.PreferredAltitude + PullUpClimbHeight;
		EnterPhase(EEnemyFlyingCombatPhase::PullUp);
	}
}

void UFlyingMovementMode::TickPullUp(float DeltaTime)
{
	const FVector DesiredVel = SteerToward(PullUpPoint, Params.MaxSpeed * 1.25f);
	MoveOwnerToward(DeltaTime, DesiredVel, false);
	FaceVelocity(DeltaTime, CurrentVelocity);

	const float Dist = FVector::Dist(OwnerPawn->GetActorLocation(), PullUpPoint);
	if (Dist <= Params.AcceptanceRadius * 1.5f)
	{
		const FVector Focus = GetFocusLocation();
		const FVector Rel = OwnerPawn->GetActorLocation() - Focus;
		OrbitAngle = FMath::Atan2(Rel.Y, Rel.X);
		EnterPhase(EEnemyFlyingCombatPhase::Orbit);
		ResetDiveTimer();
	}
}

void UFlyingMovementMode::TickMovement(float DeltaTime)
{
	if (!OwnerPawn || MovementState == EEnemyMovementState::Carried)
	{
		return;
	}

	if (!HasValidFocus())
	{
		// Idle soar near home / preferred altitude.
		const float Time = OwnerPawn->GetWorld() ? OwnerPawn->GetWorld()->GetTimeSeconds() : 0.0f;
		FVector IdlePoint = OwnerPawn->GetHomeLocation();
		IdlePoint.Z += Params.PreferredAltitude;
		IdlePoint += ComputeSwayOffset(Time) * 0.6f;
		MoveOwnerToward(DeltaTime, SteerToward(IdlePoint, Params.MaxSpeed * 0.45f), false);
		FaceVelocity(DeltaTime, CurrentVelocity);
		return;
	}

	// Keep focus location fresh for modes that only get MoveGoal updates.
	if (AActor* Focus = FocusActor.Get())
	{
		MoveGoal = Focus->GetActorLocation();
		bHasMoveGoal = true;
	}

	switch (CombatPhase)
	{
	case EEnemyFlyingCombatPhase::Approach:
		TickApproach(DeltaTime);
		break;
	case EEnemyFlyingCombatPhase::Orbit:
		TickOrbit(DeltaTime);
		break;
	case EEnemyFlyingCombatPhase::Dive:
		TickDive(DeltaTime);
		break;
	case EEnemyFlyingCombatPhase::PullUp:
		TickPullUp(DeltaTime);
		break;
	}

	if (bDrawFlightDebug || (OwnerPawn && OwnerPawn->bDrawDebug))
	{
		if (UWorld* World = OwnerPawn->GetWorld())
		{
			const FVector Loc = OwnerPawn->GetActorLocation();
			DrawDebugString(World, Loc + FVector(0, 0, 140),
				FString::Printf(TEXT("Flight: %s"), *UEnum::GetValueAsString(CombatPhase)),
				nullptr, FColor::Yellow, 0.0f, true);

			if (CombatPhase == EEnemyFlyingCombatPhase::Dive)
			{
				DrawDebugLine(World, Loc, DiveAimPoint, FColor::Magenta, false, -1.0f, 0, 2.0f);
				DrawDebugSphere(World, DiveAimPoint, DiveHitRadius, 12, FColor::Red, false, -1.0f);
			}
		}
	}
}
