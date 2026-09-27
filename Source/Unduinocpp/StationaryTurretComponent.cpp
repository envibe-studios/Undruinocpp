// StationaryTurretComponent implementation

#include "StationaryTurretComponent.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Components/SceneComponent.h"
#include "Components/PrimitiveComponent.h"

static USceneComponent* ResolveSceneComponent(const UActorComponent* OwnerComp, const FComponentReference& Ref)
{
	if (!OwnerComp)
	{
		return nullptr;
	}

	AActor* Owner = OwnerComp->GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	UActorComponent* Comp = Ref.GetComponent(Owner);
	return Cast<USceneComponent>(Comp);
}

static USceneComponent* FindSceneComponentByTagOrName(AActor* Owner, FName TagOrName)
{
	if (!Owner || TagOrName.IsNone())
	{
		return nullptr;
	}

	TArray<UActorComponent*> Components;
	Owner->GetComponents(USceneComponent::StaticClass(), Components);
	for (UActorComponent* C : Components)
	{
		if (USceneComponent* SC = Cast<USceneComponent>(C))
		{
			if (SC->ComponentHasTag(TagOrName))
			{
				return SC;
			}
			// Convenience: users often put the component variable name in the "tag" list.
			if (SC->GetFName() == TagOrName)
			{
				return SC;
			}
		}
	}
	return nullptr;
}

static USceneComponent* FindSceneComponentByName(AActor* Owner, FName Name)
{
	if (!Owner || Name.IsNone())
	{
		return nullptr;
	}

	TArray<UActorComponent*> Components;
	Owner->GetComponents(USceneComponent::StaticClass(), Components);
	for (UActorComponent* C : Components)
	{
		if (USceneComponent* SC = Cast<USceneComponent>(C))
		{
			if (SC->GetFName() == Name)
			{
				return SC;
			}
		}
	}
	return nullptr;
}

static void AddUniqueNonNull(TArray<TObjectPtr<USceneComponent>>& Arr, USceneComponent* Comp)
{
	if (!Comp)
	{
		return;
	}
	Arr.AddUnique(Comp);
}

UStationaryTurretComponent::UStationaryTurretComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UStationaryTurretComponent::BeginPlay()
{
	Super::BeginPlay();

	ResolveSetupComponents();

	if (ResolvedYawComponent)
	{
		InitialYawRelative = ResolvedYawComponent->GetRelativeRotation();
	}
	if (ResolvedPitchComponent)
	{
		InitialPitchRelative = ResolvedPitchComponent->GetRelativeRotation();
	}

	TimeSinceLastScan = TargetScanInterval; // force immediate scan
	FireCooldown = 0.0f;
	NextMuzzleIndex = 0;
	bHasSmoothedAimPoint = false;
}

void UStationaryTurretComponent::ResolveSetupComponents()
{
	AActor* Owner = GetOwner();

	ResolvedYawComponent = ResolveSceneComponent(this, YawComponent);
	ResolvedPitchComponent = ResolveSceneComponent(this, PitchComponent);
	ResolvedMuzzleComponent = ResolveSceneComponent(this, MuzzleComponent);

	ResolvedMuzzleComponents.Reset();
	for (const FComponentReference& Ref : MuzzleComponents)
	{
		AddUniqueNonNull(ResolvedMuzzleComponents, ResolveSceneComponent(this, Ref));
	}

	// Fallback: tags
	if (!ResolvedYawComponent)
	{
		ResolvedYawComponent = FindSceneComponentByTagOrName(Owner, YawComponentTag);
	}
	if (!ResolvedPitchComponent)
	{
		ResolvedPitchComponent = FindSceneComponentByTagOrName(Owner, PitchComponentTag);
	}
	if (!ResolvedMuzzleComponent)
	{
		ResolvedMuzzleComponent = FindSceneComponentByTagOrName(Owner, MuzzleComponentTag);
	}

	for (const FName& Tag : MuzzleComponentTags)
	{
		AddUniqueNonNull(ResolvedMuzzleComponents, FindSceneComponentByTagOrName(Owner, Tag));
	}

	// Fallback: names
	if (!ResolvedYawComponent)
	{
		ResolvedYawComponent = FindSceneComponentByName(Owner, YawComponentName);
	}
	if (!ResolvedPitchComponent)
	{
		ResolvedPitchComponent = FindSceneComponentByName(Owner, PitchComponentName);
	}
	if (!ResolvedMuzzleComponent)
	{
		ResolvedMuzzleComponent = FindSceneComponentByName(Owner, MuzzleComponentName);
	}

	for (const FName& Name : MuzzleComponentNames)
	{
		AddUniqueNonNull(ResolvedMuzzleComponents, FindSceneComponentByName(Owner, Name));
	}

	// Final fallback: reasonable defaults
	if (!ResolvedYawComponent && Owner)
	{
		ResolvedYawComponent = Cast<USceneComponent>(Owner->GetRootComponent());
	}
	if (!ResolvedPitchComponent)
	{
		ResolvedPitchComponent = ResolvedYawComponent;
	}

	// If user configured multiple muzzles, prefer them; otherwise keep single-muzzle fallback behavior.
	if (ResolvedMuzzleComponents.Num() == 0 && ResolvedMuzzleComponent)
	{
		ResolvedMuzzleComponents.Add(ResolvedMuzzleComponent);
	}
}

void UStationaryTurretComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Reacquire target on a cadence (cheap), validate every tick (safer)
	TimeSinceLastScan += DeltaTime;
	if (TimeSinceLastScan >= TargetScanInterval)
	{
		TimeSinceLastScan = 0.0f;
		AcquireTarget();
	}

	if (CurrentTarget.IsValid() && !IsTargetValid(CurrentTarget.Get()))
	{
		CurrentTarget.Reset();
	}

	AActor* Target = CurrentTarget.Get();
	FVector SmoothedAimWorld = FVector::ZeroVector;
	if (Target)
	{
		const FVector MuzzleLocForAim = GetMuzzleLocation();
		const FVector RawAimPoint = (bEnableTargetLeading && ProjectileSpeed > 0.0f)
			? ComputeLeadAimPoint(MuzzleLocForAim, Target)
			: GetTargetAimPoint(Target);

		if (AimSmoothingTime > 0.0f)
		{
			if (!bHasSmoothedAimPoint)
			{
				SmoothedAimPoint = RawAimPoint;
				bHasSmoothedAimPoint = true;
			}
			const float Alpha = 1.0f - FMath::Exp(-DeltaTime / FMath::Max(AimSmoothingTime, KINDA_SMALL_NUMBER));
			SmoothedAimPoint = FMath::Lerp(SmoothedAimPoint, RawAimPoint, Alpha);
			SmoothedAimWorld = SmoothedAimPoint;
		}
		else
		{
			SmoothedAimPoint = RawAimPoint;
			bHasSmoothedAimPoint = true;
			SmoothedAimWorld = SmoothedAimPoint;
		}
	}
	else
	{
		bHasSmoothedAimPoint = false;
	}

	if (Target)
	{
		UpdateAim(DeltaTime, SmoothedAimWorld);
		TryFire(DeltaTime, SmoothedAimWorld);
	}
}

void UStationaryTurretComponent::SetFiringEnabled(bool bEnabled)
{
	bFiringEnabled = bEnabled;
}

void UStationaryTurretComponent::AcquireTarget()
{
	UWorld* World = GetWorld();
	AActor* Owner = GetOwner();
	if (!World || !Owner || TargetTag.IsNone())
	{
		CurrentTarget.Reset();
		return;
	}

	TArray<AActor*> TaggedActors;
	UGameplayStatics::GetAllActorsWithTag(World, TargetTag, TaggedActors);

	AActor* Best = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();

	const FVector OwnerLoc = Owner->GetActorLocation();
	const float MaxDistSq = (TargetingRange > 0.0f) ? FMath::Square(TargetingRange) : TNumericLimits<float>::Max();

	for (AActor* Candidate : TaggedActors)
	{
		if (!IsTargetValid(Candidate))
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(OwnerLoc, Candidate->GetActorLocation());
		if (DistSq > MaxDistSq)
		{
			continue;
		}

		if (bRequireLineOfSight && !HasLineOfSightTo(Candidate))
		{
			continue;
		}

		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Best = Candidate;
		}
	}

	CurrentTarget = Best;
}

bool UStationaryTurretComponent::IsTargetValid(AActor* Candidate) const
{
	if (!Candidate)
	{
		return false;
	}

	AActor* Owner = GetOwner();
	if (Owner && Candidate == Owner)
	{
		return false;
	}

	if (Candidate->IsActorBeingDestroyed())
	{
		return false;
	}

	// Must have the tag (defensive even though acquisition uses tag query)
	if (!TargetTag.IsNone() && !Candidate->ActorHasTag(TargetTag))
	{
		return false;
	}

	return true;
}

bool UStationaryTurretComponent::HasLineOfSightTo(AActor* Candidate) const
{
	if (!Candidate)
	{
		return false;
	}

	UWorld* World = GetWorld();
	AActor* Owner = GetOwner();
	if (!World || !Owner)
	{
		return false;
	}

	const FVector Start = GetMuzzleLocation();
	const FVector End = GetTargetAimPoint(Candidate);

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(TurretLOS), /*bTraceComplex*/ false);
	Params.AddIgnoredActor(Owner);

	const bool bHit = World->LineTraceSingleByChannel(Hit, Start, End, LineOfSightChannel, Params);
	if (!bHit)
	{
		return true;
	}

	return Hit.GetActor() == Candidate;
}

USceneComponent* UStationaryTurretComponent::GetAimMuzzleSceneComponent() const
{
	if (ResolvedMuzzleComponents.Num() > 0 && ResolvedMuzzleComponents[0])
	{
		return ResolvedMuzzleComponents[0];
	}
	if (ResolvedMuzzleComponent)
	{
		return ResolvedMuzzleComponent;
	}
	if (ResolvedPitchComponent)
	{
		return ResolvedPitchComponent;
	}
	if (ResolvedYawComponent)
	{
		return ResolvedYawComponent;
	}
	AActor* Owner = GetOwner();
	return Owner ? Cast<USceneComponent>(Owner->GetRootComponent()) : nullptr;
}

USceneComponent* UStationaryTurretComponent::GetFireMuzzleSceneComponent() const
{
	if (ResolvedMuzzleComponents.Num() > 0)
	{
		const int32 UseIndex = bAlternateMuzzles ? (NextMuzzleIndex % ResolvedMuzzleComponents.Num()) : 0;
		return ResolvedMuzzleComponents.IsValidIndex(UseIndex) ? ResolvedMuzzleComponents[UseIndex] : nullptr;
	}
	return ResolvedMuzzleComponent ? ResolvedMuzzleComponent.Get() : GetAimMuzzleSceneComponent();
}

FVector UStationaryTurretComponent::GetMuzzleLocation() const
{
	if (USceneComponent* Muzzle = GetAimMuzzleSceneComponent())
	{
		return Muzzle->GetComponentLocation();
	}
	AActor* Owner = GetOwner();
	return Owner ? Owner->GetActorLocation() : FVector::ZeroVector;
}

FVector UStationaryTurretComponent::GetTargetAimPoint(AActor* Target) const
{
	if (!Target)
	{
		return FVector::ZeroVector;
	}

	FVector Point = Target->GetActorLocation();
	if (bAimAtTargetCenterOfMass)
	{
		if (const UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(Target->GetRootComponent()))
		{
			Point = Prim->GetCenterOfMass();
		}
	}
	return Point + AimOffset;
}

bool UStationaryTurretComponent::SolveInterceptTime(const FVector& RelativePos, const FVector& TargetVel, float ProjectileSpeedCmPerSec, float& OutT)
{
	// Solve |r + v t| = s t
	// (v·v - s^2) t^2 + 2(r·v) t + (r·r) = 0
	const float s = FMath::Max(ProjectileSpeedCmPerSec, 0.0f);
	if (s <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	const float a = TargetVel.SizeSquared() - s * s;
	const float b = 2.0f * FVector::DotProduct(RelativePos, TargetVel);
	const float c = RelativePos.SizeSquared();

	// If a is ~0, fall back to linear solution
	if (FMath::Abs(a) <= 1e-6f)
	{
		if (FMath::Abs(b) <= 1e-6f)
		{
			return false;
		}
		const float t = -c / b;
		if (t > 0.0f)
		{
			OutT = t;
			return true;
		}
		return false;
	}

	const float Disc = b * b - 4.0f * a * c;
	if (Disc < 0.0f)
	{
		return false;
	}

	const float SqrtDisc = FMath::Sqrt(Disc);
	const float t0 = (-b - SqrtDisc) / (2.0f * a);
	const float t1 = (-b + SqrtDisc) / (2.0f * a);

	// Choose the smallest positive time
	float t = TNumericLimits<float>::Max();
	if (t0 > 0.0f)
	{
		t = t0;
	}
	if (t1 > 0.0f)
	{
		t = FMath::Min(t, t1);
	}

	if (t == TNumericLimits<float>::Max())
	{
		return false;
	}

	OutT = t;
	return true;
}

FVector UStationaryTurretComponent::ComputeLeadAimPoint(const FVector& MuzzleLoc, AActor* Target) const
{
	const FVector TargetPos = GetTargetAimPoint(Target);
	const FVector TargetVel = Target ? Target->GetVelocity() : FVector::ZeroVector;

	float T = 0.0f;
	const FVector R = TargetPos - MuzzleLoc;
	if (SolveInterceptTime(R, TargetVel, ProjectileSpeed, T))
	{
		if (MaxLeadTime > 0.0f)
		{
			T = FMath::Min(T, MaxLeadTime);
		}
		return TargetPos + TargetVel * T;
	}
	return TargetPos;
}

void UStationaryTurretComponent::UpdateAim(float DeltaTime, const FVector& SmoothedAimWorld)
{
	if (!ResolvedYawComponent || !ResolvedPitchComponent)
	{
		return;
	}

	AActor* Target = CurrentTarget.Get();
	if (!Target)
	{
		return;
	}

	const FVector MuzzleLoc = GetMuzzleLocation();
	const FVector AimPoint = SmoothedAimWorld;

	const FVector AimDirWorld = (AimPoint - ResolvedYawComponent->GetComponentLocation()).GetSafeNormal();

	if (AimDirWorld.IsNearlyZero())
	{
		return;
	}

	// --- Desired yaw (in yaw parent space, around +Z) ---
	const USceneComponent* YawParent = ResolvedYawComponent->GetAttachParent();
	const FTransform ParentXf = YawParent ? YawParent->GetComponentTransform() : FTransform::Identity;
	const FVector AimDirParent = ParentXf.InverseTransformVectorNoScale(AimDirWorld).GetSafeNormal();

	const float DesiredYaw = FMath::RadiansToDegrees(FMath::Atan2(AimDirParent.Y, AimDirParent.X));

	// --- Desired pitch (in yaw component space, around +Y / rotator Pitch) ---
	const FVector AimDirYawSpace = ResolvedYawComponent->GetComponentTransform().InverseTransformVectorNoScale(AimDirWorld).GetSafeNormal();
	const float DesiredPitch = FMath::RadiansToDegrees(FMath::Atan2(AimDirYawSpace.Z, AimDirYawSpace.X));

	const float ClampedPitch = FMath::Clamp(DesiredPitch, MinPitchDeg, MaxPitchDeg);

	// Interp speeds
	const float YawAlpha = (YawSpeedDegPerSec <= 0.0f) ? 1.0f : FMath::Clamp(DeltaTime * (YawSpeedDegPerSec / 180.0f), 0.0f, 1.0f);
	const float PitchAlpha = (PitchSpeedDegPerSec <= 0.0f) ? 1.0f : FMath::Clamp(DeltaTime * (PitchSpeedDegPerSec / 180.0f), 0.0f, 1.0f);

	FRotator CurrentYawRel = ResolvedYawComponent->GetRelativeRotation();
	FRotator DesiredYawRel = InitialYawRelative;
	DesiredYawRel.Yaw = InitialYawRelative.Yaw + DesiredYaw;
	DesiredYawRel.Pitch = CurrentYawRel.Pitch;
	DesiredYawRel.Roll = CurrentYawRel.Roll;

	const float NewYaw = FMath::FixedTurn(CurrentYawRel.Yaw, DesiredYawRel.Yaw, YawSpeedDegPerSec * DeltaTime);
	CurrentYawRel.Yaw = NewYaw;
	ResolvedYawComponent->SetRelativeRotation(CurrentYawRel);

	FRotator CurrentPitchRel = ResolvedPitchComponent->GetRelativeRotation();
	FRotator DesiredPitchRel = InitialPitchRelative;
	DesiredPitchRel.Pitch = InitialPitchRelative.Pitch + ClampedPitch;
	DesiredPitchRel.Yaw = CurrentPitchRel.Yaw;
	DesiredPitchRel.Roll = CurrentPitchRel.Roll;

	const float NewPitch = FMath::FixedTurn(CurrentPitchRel.Pitch, DesiredPitchRel.Pitch, PitchSpeedDegPerSec * DeltaTime);
	CurrentPitchRel.Pitch = NewPitch;
	ResolvedPitchComponent->SetRelativeRotation(CurrentPitchRel);

	if (bDrawDebug)
	{
		UWorld* World = GetWorld();
		if (World)
		{
			DrawDebugLine(World, MuzzleLoc, AimPoint, FColor::Green, false, 0.0f, 0, 1.0f);
			DrawDebugSphere(World, AimPoint, 12.0f, 8, FColor::Green, false, 0.0f);
		}
	}
}

bool UStationaryTurretComponent::IsAimedAt(const USceneComponent* Muzzle, const FVector& AimPoint) const
{
	if (!Muzzle)
	{
		return false;
	}

	const FVector MuzzleLoc = Muzzle->GetComponentLocation();
	const FVector DesiredDir = (AimPoint - MuzzleLoc).GetSafeNormal();
	if (DesiredDir.IsNearlyZero())
	{
		return false;
	}

	// Arrow/mesh forward axes are not guaranteed to match the physical bore direction.
	// Use the best-aligned local axis (±X/±Y/±Z) against the desired aim vector.
	float BestDot = -2.0f;
	const FVector Axes[] = {
		Muzzle->GetForwardVector(),
		-Muzzle->GetForwardVector(),
		Muzzle->GetRightVector(),
		-Muzzle->GetRightVector(),
		Muzzle->GetUpVector(),
		-Muzzle->GetUpVector(),
	};
	for (const FVector& Axis : Axes)
	{
		const float Dot = FVector::DotProduct(Axis.GetSafeNormal(), DesiredDir);
		BestDot = FMath::Max(BestDot, Dot);
	}

	const float AngleDeg = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(BestDot, -1.0f, 1.0f)));
	return AngleDeg <= FireAngleToleranceDeg;
}

void UStationaryTurretComponent::TryFire(float DeltaTime, const FVector& SmoothedAimWorld)
{
	if (!bFiringEnabled)
	{
		return;
	}

	if (!bAutoFire)
	{
		return;
	}

	if (!ProjectileClass)
	{
		return;
	}

	AActor* Target = CurrentTarget.Get();
	if (!Target || !IsTargetValid(Target))
	{
		return;
	}

	if (bRequireLineOfSight && !HasLineOfSightTo(Target))
	{
		return;
	}

	FireCooldown -= DeltaTime;
	if (FireCooldown > 0.0f)
	{
		return;
	}

	USceneComponent* FireMuzzle = GetFireMuzzleSceneComponent();
	if (!IsAimedAt(FireMuzzle, SmoothedAimWorld))
	{
		return;
	}

	FireOnce(FireMuzzle, SmoothedAimWorld);

	const float Interval = 1.0f / FMath::Max(FireRate, 0.1f);
	FireCooldown = Interval;
}

FVector UStationaryTurretComponent::ApplySpreadToDirection(const FVector& Direction) const
{
	const float EffectiveSpread = SpreadAngleDeg * (1.0f - FMath::Clamp(Accuracy, 0.0f, 1.0f));
	if (EffectiveSpread <= 0.0f)
	{
		return Direction.GetSafeNormal();
	}

	const float HalfAngleRad = FMath::DegreesToRadians(EffectiveSpread * 0.5f);
	const float RandomAngle = FMath::FRandRange(0.0f, 2.0f * PI);
	float RandomRadius = FMath::FRandRange(0.0f, 1.0f);
	RandomRadius = FMath::Sqrt(RandomRadius);
	const float DeviationAngle = RandomRadius * HalfAngleRad;

	const FVector Dir = Direction.GetSafeNormal();

	FVector Right = FVector::CrossProduct(Dir, FVector::UpVector);
	if (Right.IsNearlyZero())
	{
		Right = FVector::CrossProduct(Dir, FVector::RightVector);
	}
	Right.Normalize();
	const FVector Up = FVector::CrossProduct(Right, Dir).GetSafeNormal();

	const FVector SpreadOffset = (Right * FMath::Cos(RandomAngle) + Up * FMath::Sin(RandomAngle)) * FMath::Sin(DeviationAngle);
	const FVector SpreadDir = Dir * FMath::Cos(DeviationAngle) + SpreadOffset;
	return SpreadDir.GetSafeNormal();
}

void UStationaryTurretComponent::FireOnce(const USceneComponent* SpawnFrom, const FVector& AimPoint)
{
	UWorld* World = GetWorld();
	AActor* Owner = GetOwner();
	if (!World || !Owner || !ProjectileClass)
	{
		return;
	}

	if (!SpawnFrom)
	{
		return;
	}

	const FVector SpawnLoc = SpawnFrom->GetComponentLocation();
	const FVector BaseDir = (AimPoint - SpawnLoc).GetSafeNormal();
	const FVector ShotDir = ApplySpreadToDirection(BaseDir);
	const FRotator SpawnRot = ShotDir.Rotation();

	FActorSpawnParameters Params;
	Params.Owner = Owner;
	Params.Instigator = Owner->GetInstigator();
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AActor* Projectile = World->SpawnActor<AActor>(ProjectileClass, SpawnLoc, SpawnRot, Params);
	if (!Projectile)
	{
		return;
	}

	if (bAlternateMuzzles && ResolvedMuzzleComponents.Num() > 0)
	{
		NextMuzzleIndex = (NextMuzzleIndex + 1) % ResolvedMuzzleComponents.Num();
	}

	// If the projectile has a ProjectileMovementComponent, drive it.
	if (UProjectileMovementComponent* PMC = Projectile->FindComponentByClass<UProjectileMovementComponent>())
	{
		if (ProjectileSpeed > 0.0f)
		{
			PMC->InitialSpeed = ProjectileSpeed;
			PMC->MaxSpeed = FMath::Max(PMC->MaxSpeed, ProjectileSpeed);
			PMC->Velocity = ShotDir * ProjectileSpeed;
		}
	}
	else if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(Projectile->GetRootComponent()))
	{
		if (Prim->IsSimulatingPhysics() && ProjectileSpeed > 0.0f)
		{
			Prim->SetPhysicsLinearVelocity(ShotDir * ProjectileSpeed);
		}
	}

	if (bDrawDebug)
	{
		DrawDebugLine(World, SpawnLoc, SpawnLoc + ShotDir * 500.0f, FColor::Red, false, 0.25f, 0, 1.5f);
	}
}

