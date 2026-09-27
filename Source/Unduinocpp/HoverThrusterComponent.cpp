// Hover Thruster Component Implementation

#include "HoverThrusterComponent.h"
#include "GameFramework/Actor.h"
#include "Components/PrimitiveComponent.h"
#include "DrawDebugHelpers.h"
#include "Net/UnrealNetwork.h"

UHoverThrusterComponent::UHoverThrusterComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;

	// Enable replication for networked games
	SetIsReplicatedByDefault(true);
}

void UHoverThrusterComponent::BeginPlay()
{
	Super::BeginPlay();

	// Initialize previous hitpoints tracking
	PreviousHitpoints = CurrentHitpoints;

	// Update initial health state
	UpdateHealthState();
}

void UHoverThrusterComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Process auto-repair if enabled
	if (bAutoRepair)
	{
		ProcessAutoRepair(DeltaTime);
	}

	// Process malfunction/sputter
	if (bEnableMalfunction)
	{
		ProcessMalfunction(DeltaTime);
	}
}

void UHoverThrusterComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UHoverThrusterComponent, CurrentHitpoints);
}

void UHoverThrusterComponent::OnRep_CurrentHitpoints()
{
	// Calculate what changed
	float Delta = CurrentHitpoints - PreviousHitpoints;

	if (Delta < 0.0f)
	{
		// Damage was applied
		OnThrusterDamaged.Broadcast(CurrentHitpoints, -Delta);
	}
	else if (Delta > 0.0f)
	{
		// Healing occurred
		OnThrusterHealed.Broadcast(CurrentHitpoints, Delta);
	}

	// Update health state
	UpdateHealthState();

	// Update tracking
	PreviousHitpoints = CurrentHitpoints;
}

// ============================================================================
// DAMAGE / HEALTH FUNCTIONS
// ============================================================================

float UHoverThrusterComponent::ApplyDamage(float DamageAmount)
{
	if (!bCanBeDamaged || DamageAmount <= 0.0f || IsDestroyed())
	{
		return CurrentHitpoints;
	}

	float OldHitpoints = CurrentHitpoints;
	CurrentHitpoints = FMath::Max(0.0f, CurrentHitpoints - DamageAmount);

	if (CurrentHitpoints != OldHitpoints)
	{
		OnThrusterDamaged.Broadcast(CurrentHitpoints, DamageAmount);
		UpdateHealthState();
		PreviousHitpoints = CurrentHitpoints;

		// Check for destruction
		if (CurrentHitpoints <= 0.0f)
		{
			OnThrusterDestroyed.Broadcast();
		}
	}

	return CurrentHitpoints;
}

float UHoverThrusterComponent::Heal(float HealAmount)
{
	if (HealAmount <= 0.0f)
	{
		return CurrentHitpoints;
	}

	bool bWasDestroyed = IsDestroyed();
	float OldHitpoints = CurrentHitpoints;
	CurrentHitpoints = FMath::Min(MaxHitpoints, CurrentHitpoints + HealAmount);

	if (CurrentHitpoints != OldHitpoints)
	{
		OnThrusterHealed.Broadcast(CurrentHitpoints, HealAmount);
		UpdateHealthState();
		PreviousHitpoints = CurrentHitpoints;

		// Check for repair from destroyed
		if (bWasDestroyed && CurrentHitpoints > 0.0f)
		{
			OnThrusterRepaired.Broadcast();
		}
	}

	return CurrentHitpoints;
}

void UHoverThrusterComponent::FullRepair()
{
	bool bWasDestroyed = IsDestroyed();
	float HealAmount = MaxHitpoints - CurrentHitpoints;

	if (HealAmount > 0.0f)
	{
		CurrentHitpoints = MaxHitpoints;
		OnThrusterHealed.Broadcast(CurrentHitpoints, HealAmount);
		UpdateHealthState();
		PreviousHitpoints = CurrentHitpoints;

		if (bWasDestroyed)
		{
			OnThrusterRepaired.Broadcast();
		}
	}
}

void UHoverThrusterComponent::SetHitpoints(float NewHitpoints)
{
	float ClampedHitpoints = FMath::Clamp(NewHitpoints, 0.0f, MaxHitpoints);

	if (ClampedHitpoints != CurrentHitpoints)
	{
		bool bWasDestroyed = IsDestroyed();
		float OldHitpoints = CurrentHitpoints;
		CurrentHitpoints = ClampedHitpoints;

		// Fire appropriate events
		if (ClampedHitpoints < OldHitpoints)
		{
			OnThrusterDamaged.Broadcast(CurrentHitpoints, OldHitpoints - ClampedHitpoints);
		}
		else
		{
			OnThrusterHealed.Broadcast(CurrentHitpoints, ClampedHitpoints - OldHitpoints);
		}

		UpdateHealthState();
		PreviousHitpoints = CurrentHitpoints;

		// Check state transitions
		if (ClampedHitpoints <= 0.0f && !bWasDestroyed)
		{
			OnThrusterDestroyed.Broadcast();
		}
		else if (ClampedHitpoints > 0.0f && bWasDestroyed)
		{
			OnThrusterRepaired.Broadcast();
		}
	}
}

float UHoverThrusterComponent::GetHealthPercent() const
{
	if (MaxHitpoints <= 0.0f)
	{
		return 0.0f;
	}
	return (CurrentHitpoints / MaxHitpoints) * 100.0f;
}

EThrusterHealthState UHoverThrusterComponent::GetHealthState() const
{
	return CurrentHealthState;
}

bool UHoverThrusterComponent::IsDestroyed() const
{
	return CurrentHitpoints <= 0.0f;
}

bool UHoverThrusterComponent::IsSputtering() const
{
	return bIsSputtering;
}

void UHoverThrusterComponent::UpdateHealthState()
{
	EThrusterHealthState OldState = CurrentHealthState;
	float HealthPercent = GetHealthPercent();

	if (HealthPercent <= 0.0f)
	{
		CurrentHealthState = EThrusterHealthState::Destroyed;
	}
	else if (HealthPercent < FailingThreshold)
	{
		CurrentHealthState = EThrusterHealthState::Failing;
	}
	else if (HealthPercent < CriticalThreshold)
	{
		CurrentHealthState = EThrusterHealthState::Critical;
	}
	else if (HealthPercent < DamagedThreshold)
	{
		CurrentHealthState = EThrusterHealthState::Damaged;
	}
	else
	{
		CurrentHealthState = EThrusterHealthState::Healthy;
	}

	if (OldState != CurrentHealthState)
	{
		OnThrusterStateChanged.Broadcast(OldState, CurrentHealthState);
	}
}

void UHoverThrusterComponent::ProcessAutoRepair(float DeltaTime)
{
	// Don't auto-repair if destroyed
	if (IsDestroyed() || CurrentHitpoints >= MaxHitpoints)
	{
		return;
	}

	Heal(AutoRepairRate * DeltaTime);
}

void UHoverThrusterComponent::ProcessMalfunction(float DeltaTime)
{
	// Update existing sputter
	if (bIsSputtering)
	{
		SputterTimeRemaining -= DeltaTime;
		if (SputterTimeRemaining <= 0.0f)
		{
			bIsSputtering = false;
			SputterTimeRemaining = 0.0f;
		}
		return;
	}

	// Don't start new sputter if healthy or destroyed
	if (CurrentHealthState == EThrusterHealthState::Healthy ||
		CurrentHealthState == EThrusterHealthState::Destroyed)
	{
		return;
	}

	// Get sputter chance based on health state
	float SputterChance = 0.0f;
	switch (CurrentHealthState)
	{
		case EThrusterHealthState::Damaged:
			SputterChance = DamagedSputterChance;
			break;
		case EThrusterHealthState::Critical:
			SputterChance = CriticalSputterChance;
			break;
		case EThrusterHealthState::Failing:
			SputterChance = FailingSputterChance;
			break;
		default:
			break;
	}

	// Roll for sputter (convert per-second chance to per-frame)
	float FrameChance = SputterChance * DeltaTime;
	if (FMath::FRand() < FrameChance)
	{
		bIsSputtering = true;
		SputterTimeRemaining = SputterDuration;

		// Calculate sputter strength based on health
		float SputterStrength = 1.0f - (GetHealthPercent() / 100.0f);
		OnThrusterSputter.Broadcast(SputterStrength);
	}
}

// ============================================================================
// PHYSICS FUNCTIONS
// ============================================================================

float UHoverThrusterComponent::GetForceEffectiveness() const
{
	if (!bIsEnabled || IsDestroyed())
	{
		return 0.0f;
	}

	float Effectiveness = GetHealthForceMultiplier();

	// Apply sputter reduction
	if (bIsSputtering)
	{
		Effectiveness *= SputterForceMultiplier;
	}

	return Effectiveness;
}

float UHoverThrusterComponent::GetHealthForceMultiplier() const
{
	// Full force at 100% health
	// Scale down as health decreases
	// At 50% health, force is reduced
	// At 0% health, force is 0

	float HealthPercent = GetHealthPercent();

	if (HealthPercent >= 100.0f)
	{
		return 1.0f;
	}
	else if (HealthPercent >= CriticalThreshold)
	{
		// Damaged state: 100% to CriticalThreshold% health = 100% to 80% force
		float T = (HealthPercent - CriticalThreshold) / (100.0f - CriticalThreshold);
		return FMath::Lerp(0.8f, 1.0f, T);
	}
	else if (HealthPercent >= FailingThreshold)
	{
		// Critical state: CriticalThreshold% to FailingThreshold% health = 80% to 50% force
		float T = (HealthPercent - FailingThreshold) / (CriticalThreshold - FailingThreshold);
		return FMath::Lerp(0.5f, 0.8f, T);
	}
	else if (HealthPercent > 0.0f)
	{
		// Failing state: FailingThreshold% to 0% health = 50% to 10% force
		float T = HealthPercent / FailingThreshold;
		return FMath::Lerp(0.1f, 0.5f, T);
	}

	return 0.0f;
}

bool UHoverThrusterComponent::PerformGroundTrace(FHitResult& OutHit) const
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return false;
	}

	FVector TraceStart = GetComponentLocation();
	FVector TraceEnd = TraceStart - FVector::UpVector * HoverHeight * TraceDistanceMultiplier;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Owner);
	QueryParams.bTraceComplex = false;

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		OutHit,
		TraceStart,
		TraceEnd,
		TraceChannel,
		QueryParams
	);

	// Debug visualization
	if (bDrawDebug)
	{
		FColor TraceColor = bHit ? FColor::Green : FColor::Red;
		DrawDebugLine(GetWorld(), TraceStart, bHit ? OutHit.ImpactPoint : TraceEnd, TraceColor, false, -1.0f, 0, 2.0f);

		if (bHit)
		{
			DrawDebugSphere(GetWorld(), OutHit.ImpactPoint, 10.0f, 8, FColor::Yellow, false, -1.0f, 0, 1.0f);
			DrawDebugDirectionalArrow(GetWorld(), OutHit.ImpactPoint, OutHit.ImpactPoint + OutHit.ImpactNormal * 50.0f, 10.0f, FColor::Blue, false, -1.0f, 0, 2.0f);
		}
	}

	return bHit;
}

FVector UHoverThrusterComponent::CalculateHoverForce(float DeltaTime)
{
	FHitResult HitResult;
	bLastTraceHit = PerformGroundTrace(HitResult);

	if (!bLastTraceHit)
	{
		LastDistanceToGround = -1.0f;
		LastGroundNormal = FVector::UpVector;
		return FVector::ZeroVector;
	}

	LastDistanceToGround = HitResult.Distance;
	LastGroundNormal = HitResult.ImpactNormal;

	// Get force effectiveness (accounts for damage and sputter)
	float Effectiveness = GetForceEffectiveness();
	if (Effectiveness <= 0.0f)
	{
		return FVector::ZeroVector;
	}

	// Calculate compression (how much we're below target hover height)
	float Compression = HoverHeight - LastDistanceToGround;

	// Spring force (Hooke's law: F = -kx)
	float SpringForce = Compression * HoverStiffness;

	// Damping force (F = -bv)
	// Calculate vertical velocity at this point
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return FVector::ZeroVector;
	}

	UPrimitiveComponent* RootPrimitive = Cast<UPrimitiveComponent>(Owner->GetRootComponent());
	if (!RootPrimitive)
	{
		return FVector::ZeroVector;
	}

	// Get linear velocity at thruster location (includes rotational contribution)
	FVector ThrusterLocation = GetComponentLocation();
	FVector CenterOfMass = RootPrimitive->GetCenterOfMass();
	FVector AngularVelocityRad = RootPrimitive->GetPhysicsAngularVelocityInRadians();
	FVector RadiusVector = ThrusterLocation - CenterOfMass;

	// Linear velocity at thruster point = linear velocity + (angular velocity x radius)
	FVector LinearVelocity = RootPrimitive->GetPhysicsLinearVelocity();
	FVector RotationalVelocity = FVector::CrossProduct(AngularVelocityRad, RadiusVector);
	FVector VelocityAtThruster = LinearVelocity + RotationalVelocity;

	// Vertical velocity includes both linear motion and rotational contribution (pitch/roll)
	float VerticalVelocity = FVector::DotProduct(VelocityAtThruster, LastGroundNormal);

	// Combined damping: standard damping + pitch stabilization for rotational component
	float RotationalVerticalSpeed = FVector::DotProduct(RotationalVelocity, LastGroundNormal);
	float DampingForce = -VerticalVelocity * HoverDamping - RotationalVerticalSpeed * PitchStabilization;

	// Total force
	float TotalForce = SpringForce + DampingForce;

	// Clamp to max force
	TotalForce = FMath::Clamp(TotalForce, 0.0f, MaxHoverForce);

	// Apply effectiveness multiplier (damage + sputter)
	TotalForce *= Effectiveness;

	// Force direction is along ground normal
	FVector ForceVector = LastGroundNormal * TotalForce;

	return ForceVector;
}

bool UHoverThrusterComponent::ApplyHoverForce(float DeltaTime)
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return false;
	}

	UPrimitiveComponent* RootPrimitive = Cast<UPrimitiveComponent>(Owner->GetRootComponent());
	if (!RootPrimitive || !RootPrimitive->IsSimulatingPhysics())
	{
		return false;
	}

	FVector HoverForce = CalculateHoverForce(DeltaTime);

	if (!HoverForce.IsNearlyZero())
	{
		// Apply force at thruster location
		RootPrimitive->AddForceAtLocation(HoverForce, GetComponentLocation());

		// Apply angular damping for stabilization
		if (AngularDamping > 0.0f)
		{
			FVector AngularVelocity = RootPrimitive->GetPhysicsAngularVelocityInDegrees();
			FVector DampingTorque = -AngularVelocity * AngularDamping * GetForceEffectiveness();
			RootPrimitive->AddTorqueInDegrees(DampingTorque);
		}

		return true;
	}

	return false;
}

bool UHoverThrusterComponent::IsGroundDetected() const
{
	return bLastTraceHit;
}

float UHoverThrusterComponent::GetDistanceToGround() const
{
	return LastDistanceToGround;
}

FVector UHoverThrusterComponent::GetGroundNormal() const
{
	return LastGroundNormal;
}

void UHoverThrusterComponent::SetThrusterEnabled(bool bNewEnabled)
{
	bIsEnabled = bNewEnabled;
}

bool UHoverThrusterComponent::IsThrusterEnabled() const
{
	return bIsEnabled;
}
