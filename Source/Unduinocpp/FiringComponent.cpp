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

	// Capture the design-time base pose so IMU corrections produce absolute final rotations
	BasePoseQuat = GetRelativeRotation().Quaternion();

	// Initialize bullet cooldown based on rate of fire
	BulletCooldown = 0.0f;
}

void UFiringComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

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

		// Apply pull force
		if (TargetPrimitive && TargetPrimitive->IsSimulatingPhysics())
		{
			FVector PullDirection = (Origin - TargetLocation).GetSafeNormal();
			FVector PullForce = PullDirection * TractorBeamConfig.PullForce;
			TargetPrimitive->AddForce(PullForce);
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
			DrawDebugLine(GetWorld(), Origin, TargetLocation, FColor::Cyan, false, -1.0f, 0, 3.0f);
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
			DrawDebugLine(GetWorld(), Origin, TraceEnd, FColor::Blue, false, -1.0f, 0, 1.0f);
		}
	}
}

AActor* UFiringComponent::FindTractorTarget()
{
	FHitResult HitResult;
	if (PerformTrace(HitResult, TractorBeamConfig.Range, TractorBeamConfig.TraceChannel))
	{
		AActor* HitActor = HitResult.GetActor();
		if (HitActor && CanTractorActor(HitActor))
		{
			return HitActor;
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

	// Check if it has a physics-simulating primitive
	UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Actor->GetRootComponent());
	if (!Primitive || !Primitive->IsSimulatingPhysics())
	{
		return false;
	}

	// Check tag if required
	if (!TractorBeamConfig.TractorableTag.IsNone())
	{
		if (!Actor->ActorHasTag(TractorBeamConfig.TractorableTag))
		{
			return false;
		}
	}

	// Check mass limit
	if (TractorBeamConfig.MaxMass > 0.0f)
	{
		float Mass = Primitive->GetMass();
		if (Mass > TractorBeamConfig.MaxMass)
		{
			return false;
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
			DrawDebugLine(GetWorld(), Origin, TargetLocation, FColor::Green, false, -1.0f, 0, 2.0f);
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
			DrawDebugLine(GetWorld(), Origin, TraceEnd, FColor::Yellow, false, -1.0f, 0, 1.0f);

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
				DrawDebugLine(GetWorld(), Origin, ConeEnd, FColor::Yellow, false, -1.0f, 0, 0.5f);
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

	// Check tag if required
	if (!ScannerConfig.ScannableTag.IsNone())
	{
		if (!Actor->ActorHasTag(ScannerConfig.ScannableTag))
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
// IMU ORIENTATION / ZEROING
// ============================================================================

void UFiringComponent::ZeroOrientation()
{
	if (bHasLastRawQuat)
	{
		// Re-zero immediately from cached last IMU sample
		ZeroQuat = LastRawQuat;
		bHasZeroReference = true;

		// Snap weapon to base pose + mount correction so it looks correct immediately
		FQuat MountQuat = MountCorrectionOffset.Quaternion();
		FQuat SnapPose = BasePoseQuat * MountQuat;
		SnapPose.Normalize();
		SetRelativeRotation(SnapPose.Rotator());

		UE_LOG(LogTemp, Log, TEXT("FiringComponent: Orientation zeroed from cached IMU sample"));
	}
	else
	{
		// No IMU data yet — just reset the flag; do NOT auto-zero on next sample
		bHasZeroReference = false;
		ZeroQuat = FQuat::Identity;

		// Restore base pose
		SetRelativeRotation(BasePoseQuat.Rotator());

		UE_LOG(LogTemp, Log, TEXT("FiringComponent: Orientation zero requested but no IMU data yet"));
	}
}

void UFiringComponent::ApplyImuOrientation(const FQuat& RawImuQuat)
{
	// Normalize the incoming quaternion
	FQuat Raw = RawImuQuat;
	Raw.Normalize();

	// Sign-flip continuity: quaternions q and -q represent the same rotation,
	// but interpolation/smoothing can jump if the sign flips. Keep the sign
	// consistent with the previous sample by checking the dot product.
	if (bHasLastRawQuat)
	{
		if ((Raw | LastRawQuat) < 0.0f)
		{
			Raw *= -1.0f;
		}
	}
	LastRawQuat = Raw;
	bHasLastRawQuat = true;

	// Compute zeroed rotation: remove the zero-pose bias if calibrated
	// When Raw == ZeroQuat, Zeroed == Identity (weapon at neutral pose)
	FQuat Zeroed = bHasZeroReference ? (ZeroQuat.Inverse() * Raw) : Raw;
	Zeroed.Normalize();

	// Apply constant mount correction to compensate for physical mounting orientation
	FQuat MountQuat = MountCorrectionOffset.Quaternion();
	FQuat Mounted = MountQuat * Zeroed;
	Mounted.Normalize();

	// Compose with the design-time base pose to produce an absolute final rotation
	// This preserves the original "snappy absolute aim" contract:
	// IMU represents an absolute weapon pose in component space
	FQuat FinalAbs = BasePoseQuat * Mounted;
	FinalAbs.Normalize();

	// Apply immediately on packet receive — no Tick smoothing
	SetRelativeRotation(FinalAbs.Rotator());
}

bool UFiringComponent::IsOrientationZeroed() const
{
	return bHasZeroReference;
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
