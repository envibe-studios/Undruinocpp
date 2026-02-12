// Firing Component Implementation

#include "FiringComponent.h"
#include "GameFramework/Actor.h"
#include "Components/PrimitiveComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"

UFiringComponent::UFiringComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UFiringComponent::BeginPlay()
{
	Super::BeginPlay();

	// Initialize bullet cooldown based on rate of fire
	BulletCooldown = 0.0f;
}

void UFiringComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Draw debug trace line whenever firing is active
	if (bIsFiring && bDrawDebug)
	{
		FVector Origin = GetFiringOrigin();
		FVector Direction = GetFiringDirection();

		// Use the range from the current mode's config
		float DebugRange = 0.0f;
		FColor DebugColor = FColor::White;
		switch (CurrentFiringMode)
		{
			case EFiringModeType::Bullet:
				DebugRange = BulletConfig.Range;
				DebugColor = FColor::Red;
				break;
			case EFiringModeType::TractorBeam:
				DebugRange = TractorBeamConfig.Range;
				DebugColor = FColor::Cyan;
				break;
			case EFiringModeType::Scanner:
				DebugRange = ScannerConfig.Range;
				DebugColor = FColor::Green;
				break;
			default:
				DebugRange = 5000.0f;
				DebugColor = FColor::White;
				break;
		}

		FVector TraceEnd = Origin + Direction * DebugRange;

		// Perform a trace to see if we hit something
		FHitResult DebugHit;
		FCollisionQueryParams DebugQueryParams;
		DebugQueryParams.AddIgnoredActor(GetOwner());
		bool bDebugHit = GetWorld()->LineTraceSingleByChannel(
			DebugHit, Origin, TraceEnd, ECC_Visibility, DebugQueryParams);

		FVector EndPoint = bDebugHit ? DebugHit.ImpactPoint : TraceEnd;
		// Use SDPG_Foreground (depth priority 1) so the line draws on top of geometry
		DrawDebugLine(GetWorld(), Origin, EndPoint, DebugColor, false, -1.0f, 1, 2.0f);

		if (bDebugHit)
		{
			DrawDebugPoint(GetWorld(), DebugHit.ImpactPoint, 10.0f, DebugColor, false, -1.0f);
		}
	}

	if (!bIsFiring)
	{
		return;
	}

	// Process the current firing mode
	switch (CurrentFiringMode)
	{
		case EFiringModeType::Bullet:
			ProcessBulletMode(DeltaTime);
			break;

		case EFiringModeType::TractorBeam:
			ProcessTractorBeamMode(DeltaTime);
			break;

		case EFiringModeType::Scanner:
			ProcessScannerMode(DeltaTime);
			break;

		default:
			break;
	}
}

// ============================================================================
// CONTROL FUNCTIONS
// ============================================================================

void UFiringComponent::SetFiring(bool bShouldFire)
{
	if (bIsFiring == bShouldFire)
	{
		return;
	}

	bIsFiring = bShouldFire;

	if (bIsFiring)
	{
		OnFiringStarted.Broadcast(CurrentFiringMode);
	}
	else
	{
		OnFiringStopped.Broadcast(CurrentFiringMode);

		// Clean up mode-specific state
		if (CurrentFiringMode == EFiringModeType::TractorBeam && TractorTarget.IsValid())
		{
			AActor* Target = TractorTarget.Get();
			OnTractorBeamLost.Broadcast(Target);
			TractorTarget.Reset();
		}
		else if (CurrentFiringMode == EFiringModeType::Scanner && ScanTarget.IsValid())
		{
			AActor* Target = ScanTarget.Get();
			OnScanCancelled.Broadcast(Target, CurrentScanProgress);
			ScanTarget.Reset();
			CurrentScanProgress = 0.0f;
		}
	}
}

bool UFiringComponent::IsFiring() const
{
	return bIsFiring;
}

void UFiringComponent::SetFiringMode(EFiringModeType NewMode)
{
	if (CurrentFiringMode == NewMode)
	{
		return;
	}

	// Reset state from previous mode
	ResetModeState();

	EFiringModeType OldMode = CurrentFiringMode;
	CurrentFiringMode = NewMode;

	OnFiringModeChanged.Broadcast(CurrentFiringMode);
}

EFiringModeType UFiringComponent::GetFiringMode() const
{
	return CurrentFiringMode;
}

void UFiringComponent::CycleNextFiringMode()
{
	int32 CurrentIndex = static_cast<int32>(CurrentFiringMode);
	int32 NextIndex = (CurrentIndex + 1) % 3; // Cycle through Bullet, TractorBeam, Scanner
	SetFiringMode(static_cast<EFiringModeType>(NextIndex));
}

void UFiringComponent::CyclePreviousFiringMode()
{
	int32 CurrentIndex = static_cast<int32>(CurrentFiringMode);
	int32 PrevIndex = (CurrentIndex + 2) % 3; // Cycle backwards
	SetFiringMode(static_cast<EFiringModeType>(PrevIndex));
}

void UFiringComponent::ResetModeState()
{
	// Release tractor target if switching away from tractor beam
	if (TractorTarget.IsValid())
	{
		AActor* Target = TractorTarget.Get();

		// Restore original scale
		if (Target && Target->GetRootComponent())
		{
			Target->GetRootComponent()->SetWorldScale3D(TractorTargetOriginalScale);
		}

		OnTractorBeamLost.Broadcast(Target);
		TractorTarget.Reset();
		TractorTargetOriginalScale = FVector::OneVector;
	}

	// Cancel scan if switching away from scanner
	if (ScanTarget.IsValid())
	{
		OnScanCancelled.Broadcast(ScanTarget.Get(), CurrentScanProgress);
		ScanTarget.Reset();
		CurrentScanProgress = 0.0f;
		ScanLostTime = 0.0f;
	}

	// Reset bullet cooldown
	BulletCooldown = 0.0f;
}

// ============================================================================
// BULLET MODE
// ============================================================================

void UFiringComponent::ProcessBulletMode(float DeltaTime)
{
	if (!BulletConfig.bEnabled)
	{
		return;
	}

	// Update cooldown
	if (BulletCooldown > 0.0f)
	{
		BulletCooldown -= DeltaTime;
	}

	// Fire when ready
	if (BulletCooldown <= 0.0f)
	{
		// Check ammo
		if (BulletConfig.bUseAmmo && BulletConfig.CurrentAmmo <= 0)
		{
			OnAmmoEmpty.Broadcast();
			return;
		}

		FireBullet();

		// Reset cooldown based on rate of fire
		float FireInterval = 1.0f / BulletConfig.RateOfFire;
		BulletCooldown = FireInterval;
	}
}

void UFiringComponent::FireBullet()
{
	FVector Origin = GetFiringOrigin();
	FVector BaseDirection = GetFiringDirection();

	// Consume ammo
	if (BulletConfig.bUseAmmo)
	{
		BulletConfig.CurrentAmmo = FMath::Max(0, BulletConfig.CurrentAmmo - 1);
		OnAmmoChanged.Broadcast(BulletConfig.CurrentAmmo, BulletConfig.MaxAmmo);
	}

	// Fire each bullet in the burst
	for (int32 i = 0; i < BulletConfig.BulletsPerShot; i++)
	{
		FVector Direction = ApplySpread(BaseDirection, BulletConfig.SpreadAngle);

		// Broadcast bullet fired event
		OnBulletFired.Broadcast(Origin, Direction, BulletConfig.Damage, i);

		// Perform hit trace
		FHitResult HitResult;
		FVector TraceEnd = Origin + Direction * BulletConfig.Range;

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(GetOwner());
		QueryParams.bTraceComplex = true;

		bool bHit = GetWorld()->LineTraceSingleByChannel(
			HitResult,
			Origin,
			TraceEnd,
			BulletConfig.TraceChannel,
			QueryParams
		);

		if (bDrawDebug)
		{
			FColor TraceColor = bHit ? FColor::Red : FColor::Yellow;
			DrawDebugLine(GetWorld(), Origin, bHit ? HitResult.ImpactPoint : TraceEnd, TraceColor, false, 0.1f, 0, 1.0f);

			if (bHit)
			{
				DrawDebugSphere(GetWorld(), HitResult.ImpactPoint, 5.0f, 8, FColor::Red, false, 0.1f);
			}
		}

		if (bHit && HitResult.GetActor())
		{
			OnBulletHit.Broadcast(
				HitResult.GetActor(),
				HitResult.ImpactPoint,
				HitResult.ImpactNormal,
				BulletConfig.Damage,
				HitResult.GetComponent()
			);
		}
	}
}

int32 UFiringComponent::AddAmmo(int32 Amount)
{
	if (Amount <= 0)
	{
		return BulletConfig.CurrentAmmo;
	}

	int32 OldAmmo = BulletConfig.CurrentAmmo;
	BulletConfig.CurrentAmmo = FMath::Min(BulletConfig.CurrentAmmo + Amount, BulletConfig.MaxAmmo);

	if (BulletConfig.CurrentAmmo != OldAmmo)
	{
		OnAmmoChanged.Broadcast(BulletConfig.CurrentAmmo, BulletConfig.MaxAmmo);
	}

	return BulletConfig.CurrentAmmo;
}

void UFiringComponent::SetAmmo(int32 Amount)
{
	int32 OldAmmo = BulletConfig.CurrentAmmo;
	BulletConfig.CurrentAmmo = FMath::Clamp(Amount, 0, BulletConfig.MaxAmmo);

	if (BulletConfig.CurrentAmmo != OldAmmo)
	{
		OnAmmoChanged.Broadcast(BulletConfig.CurrentAmmo, BulletConfig.MaxAmmo);
	}
}

int32 UFiringComponent::GetCurrentAmmo() const
{
	return BulletConfig.CurrentAmmo;
}

int32 UFiringComponent::GetMaxAmmo() const
{
	return BulletConfig.MaxAmmo;
}

// ============================================================================
// TRACTOR BEAM MODE
// ============================================================================

void UFiringComponent::ProcessTractorBeamMode(float DeltaTime)
{
	if (!TractorBeamConfig.bEnabled)
	{
		return;
	}

	FVector Origin = GetFiringOrigin();

	// If we have a target, process pulling
	if (TractorTarget.IsValid())
	{
		AActor* Target = TractorTarget.Get();

		if (!Target || !Target->GetRootComponent())
		{
			OnTractorBeamLost.Broadcast(Target);
			TractorTarget.Reset();
			return;
		}

		UPrimitiveComponent* TargetPrimitive = Cast<UPrimitiveComponent>(Target->GetRootComponent());
		FVector TargetLocation = Target->GetActorLocation();
		float Distance = FVector::Dist(Origin, TargetLocation);

		// Check if target is still in range
		if (Distance > TractorBeamConfig.Range * 1.5f) // Allow some buffer
		{
			// Restore original scale and release
			Target->GetRootComponent()->SetWorldScale3D(TractorTargetOriginalScale);
			OnTractorBeamLost.Broadcast(Target);
			TractorTarget.Reset();
			return;
		}

		// Apply pull force or direct movement
		FVector PullDirection = (Origin - TargetLocation).GetSafeNormal();
		if (TargetPrimitive && TargetPrimitive->IsSimulatingPhysics())
		{
			FVector PullForce = PullDirection * TractorBeamConfig.PullForce;
			TargetPrimitive->AddForce(PullForce);
		}
		else
		{
			// Non-physics objects: move directly toward origin
			float PullSpeed = TractorBeamConfig.PullForce * 0.01f; // Scale force to a reasonable movement speed
			FVector NewLocation = TargetLocation + PullDirection * PullSpeed * DeltaTime;
			Target->SetActorLocation(NewLocation);
		}

		// Shrink the object as it gets closer
		FVector CurrentScale = Target->GetRootComponent()->GetComponentScale();
		float ShrinkAmount = TractorBeamConfig.ShrinkRate * DeltaTime;
		FVector NewScale = CurrentScale - FVector(ShrinkAmount);
		NewScale = NewScale.ComponentMax(FVector(TractorBeamConfig.MinScaleForCollection));
		Target->GetRootComponent()->SetWorldScale3D(NewScale);

		// Broadcast pulling event
		OnTractorBeamPulling.Broadcast(Target, Distance);

		// Debug visualization
		if (bDrawDebug)
		{
			DrawDebugLine(GetWorld(), Origin, TargetLocation, FColor::Cyan, false, -1.0f, 1, 3.0f);
			DrawDebugSphere(GetWorld(), TargetLocation, 20.0f, 8, FColor::Cyan, false, -1.0f);
		}

		// Check if object should be collected
		if (Distance <= TractorBeamConfig.CollectionDistance ||
			NewScale.GetMin() <= TractorBeamConfig.MinScaleForCollection)
		{
			// Collect the object
			OnObjectCollected.Broadcast(Target);

			// Destroy the actor
			Target->Destroy();
			TractorTarget.Reset();
		}
	}
	else
	{
		// Look for a new target
		AActor* NewTarget = FindTractorTarget();
		if (NewTarget)
		{
			TractorTarget = NewTarget;
			TractorTargetOriginalScale = NewTarget->GetRootComponent()->GetComponentScale();
			OnTractorBeamStart.Broadcast(NewTarget);
		}

		// Debug: show tractor beam searching
		if (bDrawDebug)
		{
			FVector Direction = GetFiringDirection();
			FVector TraceEnd = Origin + Direction * TractorBeamConfig.Range;
			DrawDebugLine(GetWorld(), Origin, TraceEnd, FColor::Blue, false, -1.0f, 1, 1.0f);
		}
	}
}

AActor* UFiringComponent::FindTractorTarget()
{
	FHitResult HitResult;
	if (PerformTrace(HitResult, TractorBeamConfig.Range, TractorBeamConfig.TraceChannel))
	{
		AActor* HitActor = HitResult.GetActor();
		if (HitActor)
		{
			if (bDrawDebug)
			{
				UE_LOG(LogTemp, Log, TEXT("TractorBeam: Trace hit '%s'. TractorableTags configured: %d"),
					*HitActor->GetName(), TractorBeamConfig.TractorableTags.Num());
			}

			if (CanTractorActor(HitActor))
			{
				if (bDrawDebug)
				{
					UE_LOG(LogTemp, Log, TEXT("TractorBeam: Accepted target '%s'"), *HitActor->GetName());
				}
				return HitActor;
			}
			else if (bDrawDebug)
			{
				UE_LOG(LogTemp, Warning, TEXT("TractorBeam: Trace hit '%s' but CanTractorActor returned false"), *HitActor->GetName());
			}
		}
	}
	return nullptr;
}

bool UFiringComponent::CanTractorActor(AActor* Actor) const
{
	if (!Actor || !Actor->GetRootComponent())
	{
		return false;
	}

	// Check tags if any are specified — actor must have at least one matching tag
	// Checks both Actor Tags and Component Tags on the root component
	if (TractorBeamConfig.TractorableTags.Num() > 0)
	{
		bool bHasMatchingTag = false;
		for (const FName& Tag : TractorBeamConfig.TractorableTags)
		{
			if (Actor->ActorHasTag(Tag))
			{
				bHasMatchingTag = true;
				break;
			}
			// Also check component tags on the root component
			if (Actor->GetRootComponent()->ComponentHasTag(Tag))
			{
				bHasMatchingTag = true;
				break;
			}
		}
		if (!bHasMatchingTag)
		{
			if (bDrawDebug)
			{
				TArray<FString> ComponentTagStrings;
				for (const FName& T : Actor->GetRootComponent()->ComponentTags)
				{
					ComponentTagStrings.Add(T.ToString());
				}
				UE_LOG(LogTemp, Warning, TEXT("TractorBeam: Actor '%s' rejected - missing required tag. Actor tags: [%s], Component tags: [%s], Required tags: [%s]"),
					*Actor->GetName(),
					*FString::JoinBy(Actor->Tags, TEXT(", "), [](const FName& T) { return T.ToString(); }),
					*FString::Join(ComponentTagStrings, TEXT(", ")),
					*FString::JoinBy(TractorBeamConfig.TractorableTags, TEXT(", "), [](const FName& T) { return T.ToString(); }));
			}
			return false;
		}
	}
	else
	{
		// No tags configured — all actors pass tag filter
		if (bDrawDebug)
		{
			UE_LOG(LogTemp, Verbose, TEXT("TractorBeam: No TractorableTags configured — all actors are eligible. Actor: '%s'"), *Actor->GetName());
		}
	}

	// Check mass limit if the object has a physics-simulating primitive
	if (TractorBeamConfig.MaxMass > 0.0f)
	{
		UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Actor->GetRootComponent());
		if (Primitive && Primitive->IsSimulatingPhysics())
		{
			float Mass = Primitive->GetMass();
			if (Mass > TractorBeamConfig.MaxMass)
			{
				return false;
			}
		}
	}

	return true;
}

bool UFiringComponent::HasTractorTarget() const
{
	return TractorTarget.IsValid();
}

AActor* UFiringComponent::GetTractorTarget() const
{
	return TractorTarget.Get();
}

void UFiringComponent::ReleaseTractorTarget()
{
	if (TractorTarget.IsValid())
	{
		AActor* Target = TractorTarget.Get();

		// Restore original scale
		if (Target && Target->GetRootComponent())
		{
			Target->GetRootComponent()->SetWorldScale3D(TractorTargetOriginalScale);
		}

		OnTractorBeamLost.Broadcast(Target);
		TractorTarget.Reset();
		TractorTargetOriginalScale = FVector::OneVector;
	}
}

// ============================================================================
// SCANNER MODE
// ============================================================================

void UFiringComponent::ProcessScannerMode(float DeltaTime)
{
	if (!ScannerConfig.bEnabled)
	{
		return;
	}

	FVector Origin = GetFiringOrigin();
	FVector Direction = GetFiringDirection();

	// If we have a scan target, continue scanning
	if (ScanTarget.IsValid())
	{
		AActor* Target = ScanTarget.Get();

		if (!Target)
		{
			OnScanCancelled.Broadcast(nullptr, CurrentScanProgress);
			ScanTarget.Reset();
			CurrentScanProgress = 0.0f;
			return;
		}

		FVector TargetLocation = Target->GetActorLocation();
		float Distance = FVector::Dist(Origin, TargetLocation);

		// Check if target is still in range
		bool bInRange = Distance <= ScannerConfig.Range;

		// Check if target is still in cone
		FVector ToTarget = (TargetLocation - Origin).GetSafeNormal();
		float Angle = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(Direction, ToTarget)));
		bool bInCone = Angle <= ScannerConfig.ScanConeAngle;

		bool bTargetValid = bInRange && bInCone;

		if (!bTargetValid)
		{
			if (ScannerConfig.bRequireContinuousLock)
			{
				// Immediately cancel scan
				OnScanCancelled.Broadcast(Target, CurrentScanProgress);
				ScanTarget.Reset();
				CurrentScanProgress = 0.0f;
			}
			else
			{
				// Track time since target lost
				ScanLostTime += DeltaTime;
				if (ScanLostTime >= ScannerConfig.ScanResetDelay)
				{
					OnScanCancelled.Broadcast(Target, CurrentScanProgress);
					ScanTarget.Reset();
					CurrentScanProgress = 0.0f;
					ScanLostTime = 0.0f;
				}
			}
			return;
		}

		// Reset lost time if target is valid
		ScanLostTime = 0.0f;

		// Progress the scan
		float ProgressIncrement = DeltaTime / ScannerConfig.ScanDuration;
		CurrentScanProgress = FMath::Min(CurrentScanProgress + ProgressIncrement, 1.0f);

		float TimeRemaining = (1.0f - CurrentScanProgress) * ScannerConfig.ScanDuration;

		// Broadcast scanning progress
		OnScanning.Broadcast(Target, CurrentScanProgress, TimeRemaining);

		// Debug visualization
		if (bDrawDebug)
		{
			DrawDebugLine(GetWorld(), Origin, TargetLocation, FColor::Green, false, -1.0f, 1, 2.0f);
			DrawDebugSphere(GetWorld(), TargetLocation, 30.0f * CurrentScanProgress + 10.0f, 12, FColor::Green, false, -1.0f);
		}

		// Check for scan complete
		if (CurrentScanProgress >= 1.0f)
		{
			OnScanComplete.Broadcast(Target);
			ScanTarget.Reset();
			CurrentScanProgress = 0.0f;
		}
	}
	else
	{
		// Look for a new scan target
		AActor* NewTarget = FindScanTarget();
		if (NewTarget)
		{
			ScanTarget = NewTarget;
			CurrentScanProgress = 0.0f;
			ScanLostTime = 0.0f;
			OnScanStart.Broadcast(NewTarget);
		}

		// Debug: show scanner cone
		if (bDrawDebug)
		{
			FVector TraceEnd = Origin + Direction * ScannerConfig.Range;
			DrawDebugLine(GetWorld(), Origin, TraceEnd, FColor::Yellow, false, -1.0f, 1, 1.0f);

			// Draw cone edges
			float ConeRad = FMath::DegreesToRadians(ScannerConfig.ScanConeAngle);
			FVector Right = FVector::CrossProduct(Direction, FVector::UpVector).GetSafeNormal();
			FVector Up = FVector::CrossProduct(Right, Direction).GetSafeNormal();

			for (int32 i = 0; i < 8; i++)
			{
				float AngleStep = (2.0f * PI * i) / 8.0f;
				FVector ConeDir = Direction + (FMath::Sin(AngleStep) * Right + FMath::Cos(AngleStep) * Up) * FMath::Tan(ConeRad);
				ConeDir.Normalize();
				FVector ConeEnd = Origin + ConeDir * ScannerConfig.Range;
				DrawDebugLine(GetWorld(), Origin, ConeEnd, FColor::Yellow, false, -1.0f, 1, 0.5f);
			}
		}
	}
}

AActor* UFiringComponent::FindScanTarget()
{
	FHitResult HitResult;
	if (PerformTrace(HitResult, ScannerConfig.Range, ScannerConfig.TraceChannel))
	{
		AActor* HitActor = HitResult.GetActor();
		if (HitActor && CanScanActor(HitActor))
		{
			return HitActor;
		}
	}
	return nullptr;
}

bool UFiringComponent::CanScanActor(AActor* Actor) const
{
	if (!Actor)
	{
		return false;
	}

	// Check tags if any are specified — actor must have at least one matching tag
	// Checks both Actor Tags and Component Tags on the root component
	if (ScannerConfig.ScannableTags.Num() > 0)
	{
		bool bHasMatchingTag = false;
		for (const FName& Tag : ScannerConfig.ScannableTags)
		{
			if (Actor->ActorHasTag(Tag))
			{
				bHasMatchingTag = true;
				break;
			}
			// Also check component tags on the root component
			if (Actor->GetRootComponent() && Actor->GetRootComponent()->ComponentHasTag(Tag))
			{
				bHasMatchingTag = true;
				break;
			}
		}
		if (!bHasMatchingTag)
		{
			return false;
		}
	}

	return true;
}

bool UFiringComponent::HasScanTarget() const
{
	return ScanTarget.IsValid();
}

AActor* UFiringComponent::GetScanTarget() const
{
	return ScanTarget.Get();
}

float UFiringComponent::GetScanProgress() const
{
	return CurrentScanProgress;
}

void UFiringComponent::CancelScan()
{
	if (ScanTarget.IsValid())
	{
		OnScanCancelled.Broadcast(ScanTarget.Get(), CurrentScanProgress);
		ScanTarget.Reset();
		CurrentScanProgress = 0.0f;
		ScanLostTime = 0.0f;
	}
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

bool UFiringComponent::PerformTrace(FHitResult& OutHit, float Range, ECollisionChannel Channel) const
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return false;
	}

	FVector Origin = GetFiringOrigin();
	FVector Direction = GetFiringDirection();
	FVector TraceEnd = Origin + Direction * Range;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Owner);
	QueryParams.bTraceComplex = false;

	return GetWorld()->LineTraceSingleByChannel(
		OutHit,
		Origin,
		TraceEnd,
		Channel,
		QueryParams
	);
}

FVector UFiringComponent::GetFiringDirection() const
{
	return GetForwardVector();
}

FVector UFiringComponent::GetFiringOrigin() const
{
	return GetComponentLocation();
}

FVector UFiringComponent::ApplySpread(const FVector& Direction, float SpreadAngle) const
{
	if (SpreadAngle <= 0.0f)
	{
		return Direction;
	}

	// Generate random angles within the spread cone
	float HalfAngleRad = FMath::DegreesToRadians(SpreadAngle * 0.5f);

	// Random angle around the direction vector
	float RandomAngle = FMath::FRandRange(0.0f, 2.0f * PI);
	// Random deviation from center (weighted toward center for more natural spread)
	float RandomRadius = FMath::FRandRange(0.0f, 1.0f);
	RandomRadius = FMath::Sqrt(RandomRadius); // Uniform distribution in cone
	float DeviationAngle = RandomRadius * HalfAngleRad;

	// Create perpendicular vectors
	FVector Right = FVector::CrossProduct(Direction, FVector::UpVector);
	if (Right.IsNearlyZero())
	{
		Right = FVector::CrossProduct(Direction, FVector::RightVector);
	}
	Right.Normalize();
	FVector Up = FVector::CrossProduct(Right, Direction);
	Up.Normalize();

	// Apply deviation
	FVector SpreadOffset = (Right * FMath::Cos(RandomAngle) + Up * FMath::Sin(RandomAngle)) * FMath::Sin(DeviationAngle);
	FVector SpreadDirection = Direction * FMath::Cos(DeviationAngle) + SpreadOffset;

	return SpreadDirection.GetSafeNormal();
}

// ============================================================================
// WEAPON IMU
// ============================================================================

void UFiringComponent::ApplyImuOrientation(const FQuat& RawImuQuat)
{
	FQuat Raw = RawImuQuat;
	Raw.Normalize();

	FRotator RawRot = Raw.Rotator();
	FRotator FinalRot = RawRot + ManualAimOffset;
	SetRelativeRotation(FinalRot);
}

// ============================================================================
// WEAPON MAG INTEGRATION
// ============================================================================

void UFiringComponent::ApplyWeaponMagConfig(
	bool bActive,
	uint8 FiringMode,
	float Damage,
	float RateOfFire,
	float SpreadAngle,
	int32 BulletsPerShot,
	int32 MaxAmmo,
	int32 CurrentAmmo,
	float Range,
	float TractorPullForce,
	float ScanDuration
)
{
	// If not active, log and skip applying config
	if (!bActive)
	{
		UE_LOG(LogTemp, Log, TEXT("FiringComponent: WeaponMag is not active, skipping config apply"));
		return;
	}

	// Set the firing mode
	EFiringModeType NewMode = static_cast<EFiringModeType>(FMath::Clamp(static_cast<int32>(FiringMode), 0, 3));
	SetFiringMode(NewMode);

	// Apply bullet mode config
	BulletConfig.Damage = Damage;
	BulletConfig.RateOfFire = FMath::Max(0.1f, RateOfFire);
	BulletConfig.SpreadAngle = FMath::Clamp(SpreadAngle, 0.0f, 45.0f);
	BulletConfig.BulletsPerShot = FMath::Max(1, BulletsPerShot);
	BulletConfig.MaxAmmo = FMath::Max(1, MaxAmmo);
	// Use CurrentAmmo if provided (>= 0), otherwise reload to full
	BulletConfig.CurrentAmmo = (CurrentAmmo >= 0) ? FMath::Clamp(CurrentAmmo, 0, BulletConfig.MaxAmmo) : BulletConfig.MaxAmmo;
	BulletConfig.Range = FMath::Max(0.0f, Range);

	// Apply tractor beam config
	TractorBeamConfig.PullForce = FMath::Max(0.0f, TractorPullForce);
	TractorBeamConfig.Range = FMath::Max(0.0f, Range);

	// Apply scanner config
	ScannerConfig.ScanDuration = FMath::Max(0.1f, ScanDuration);
	ScannerConfig.Range = FMath::Max(0.0f, Range);

	// Broadcast ammo changed event since we reloaded
	OnAmmoChanged.Broadcast(BulletConfig.CurrentAmmo, BulletConfig.MaxAmmo);

	UE_LOG(LogTemp, Log, TEXT("FiringComponent: Applied WeaponMag config - Mode: %d, Damage: %.1f, RoF: %.1f, Ammo: %d/%d"),
		static_cast<int32>(NewMode), Damage, RateOfFire, BulletConfig.CurrentAmmo, MaxAmmo);
}
