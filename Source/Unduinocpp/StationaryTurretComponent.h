// StationaryTurretComponent - aim + fire controller for a 2-axis turret (yaw parent, pitch child)

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "StationaryTurretComponent.generated.h"

class USceneComponent;
class UPrimitiveComponent;

UCLASS(ClassGroup=(Weapon), meta=(BlueprintSpawnableComponent), BlueprintType, Blueprintable)
class UNDUINOCPP_API UStationaryTurretComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UStationaryTurretComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ============================================================================
	// TURRET SETUP
	// ============================================================================

	/** Component that yaws left/right (parent). Pick the yaw static mesh component from the owning BP. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret|Setup", meta=(AllowedClasses="/Script/Engine.SceneComponent", UseComponentPicker))
	FComponentReference YawComponent;

	/** Component that pitches up/down (child of yaw component). Pick the pitch static mesh component from the owning BP. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret|Setup", meta=(AllowedClasses="/Script/Engine.SceneComponent", UseComponentPicker))
	FComponentReference PitchComponent;

	/** Where projectiles spawn from. Pick a scene component (e.g., Arrow) from the owning BP. Optional. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret|Setup", meta=(AllowedClasses="/Script/Engine.SceneComponent", UseComponentPicker))
	FComponentReference MuzzleComponent;

	/** Optional: multiple muzzles. If any are set, the turret will fire from these (round-robin). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret|Setup", meta=(AllowedClasses="/Script/Engine.SceneComponent", UseComponentPicker))
	TArray<FComponentReference> MuzzleComponents;

	/**
	 * Fallback wiring by Component Tag OR component variable name (recommended if the picker list is empty).
	 * Preferred: set a Component Tag on the mesh/arrow in your BP (Details -> Tags -> Component Tags).
	 * Convenience: you can also put the component's variable name here (e.g. "Yaw", "Pitch", "MuzzleL") and it will match by name.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret|Setup|Fallback")
	FName YawComponentTag = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret|Setup|Fallback")
	FName PitchComponentTag = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret|Setup|Fallback")
	FName MuzzleComponentTag = NAME_None;

	/** Optional: multiple muzzle tags/names (round-robin). Matches Component Tags first, then component variable names. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret|Setup|Fallback")
	TArray<FName> MuzzleComponentTags;

	/**
	 * Fallback wiring by Component Name (advanced).
	 * Use the component's variable name in the BP (e.g. "Yaw", "Pitch", "BarrelL").
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret|Setup|Fallback")
	FName YawComponentName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret|Setup|Fallback")
	FName PitchComponentName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret|Setup|Fallback")
	FName MuzzleComponentName = NAME_None;

	/** Optional: multiple muzzle names (round-robin). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret|Setup|Fallback")
	TArray<FName> MuzzleComponentNames;

	/** Tag that eligible targets must have. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret|Targeting")
	FName TargetTag = FName("Targetable");

	/**
	 * When true, prefer the possessing Enemy AI blackboard TargetActor over tag scanning.
	 * Falls back to tag scan if no AI target is available.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret|Targeting|EnemyAI")
	bool bUseEnemyAITarget = false;

	/**
	 * When aiming at a ship, prefer individual components (thrusters / Engine-tagged parts)
	 * over actor center of mass. Matches Enemy AI dive targeting.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret|Targeting|EnemyAI")
	bool bPrioritizeShipParts = true;

	/** Component tags treated as high-priority aim points (e.g. engines). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret|Targeting|EnemyAI")
	TArray<FName> PriorityPartTags;

	/** How often to re-pick a ship part aim point while locked on a target (seconds). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret|Targeting|EnemyAI", meta = (ClampMin = "0.05"))
	float ShipPartReselectInterval = 1.25f;

	/** Max distance to consider targets (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret|Targeting", meta = (ClampMin = "0.0"))
	float TargetingRange = 15000.0f;

	/** How often to rescan for the closest target (seconds). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret|Targeting", meta = (ClampMin = "0.05"))
	float TargetScanInterval = 0.25f;

	/** If true, requires a visibility trace to the target before locking/firing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret|Targeting")
	bool bRequireLineOfSight = true;

	/** Trace channel used for line-of-sight checks. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret|Targeting", meta = (EditCondition = "bRequireLineOfSight"))
	TEnumAsByte<ECollisionChannel> LineOfSightChannel = ECC_Visibility;

	/** Current target actor (read-only). */
	UFUNCTION(BlueprintPure, Category = "Turret|Targeting")
	AActor* GetCurrentTarget() const { return CurrentTarget.Get(); }

	/** Force a specific target (used by Enemy AI bridge / tests). */
	UFUNCTION(BlueprintCallable, Category = "Turret|Targeting")
	void SetExternalTarget(AActor* NewTarget);

	UFUNCTION(BlueprintCallable, Category = "Turret|Targeting")
	void ClearCurrentTarget();

	// ============================================================================
	// AIMING
	// ============================================================================

	/** Yaw speed (deg/sec). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret|Aiming", meta = (ClampMin = "0.0"))
	float YawSpeedDegPerSec = 180.0f;

	/** Pitch speed (deg/sec). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret|Aiming", meta = (ClampMin = "0.0"))
	float PitchSpeedDegPerSec = 120.0f;

	/** Minimum pitch angle (degrees, relative to initial pitch). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret|Aiming", meta = (ClampMin = "-89.9", ClampMax = "89.9"))
	float MinPitchDeg = -10.0f;

	/** Maximum pitch angle (degrees, relative to initial pitch). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret|Aiming", meta = (ClampMin = "-89.9", ClampMax = "89.9"))
	float MaxPitchDeg = 45.0f;

	/** If true, leads moving targets using target velocity and ProjectileSpeed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret|Aiming")
	bool bEnableTargetLeading = true;

	/** Aim point smoothing time constant (seconds). 0 = no smoothing. Helps reduce jitter. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret|Aiming", meta=(ClampMin="0.0"))
	float AimSmoothingTime = 0.08f;

	/** Clamp lead time (seconds). Prevents extreme/unstable lead solutions. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret|Aiming", meta=(ClampMin="0.0"))
	float MaxLeadTime = 2.0f;

	/** If true, uses the target's root/velocity; otherwise aims at actor origin. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret|Aiming")
	bool bAimAtTargetCenterOfMass = true;

	/** Additional world-space offset applied to the aim point (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret|Aiming")
	FVector AimOffset = FVector::ZeroVector;

	// ============================================================================
	// FIRING
	// ============================================================================

	/** If true, turret will automatically fire when it has a valid target and is aimed sufficiently. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret|Firing")
	bool bAutoFire = true;

	/** If true and multiple muzzles are configured, cycles through them each shot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret|Firing")
	bool bAlternateMuzzles = true;

	/** Shots per second. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret|Firing", meta = (ClampMin = "0.1"))
	float FireRate = 5.0f;

	/** Projectile actor class to spawn. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret|Firing")
	TSubclassOf<AActor> ProjectileClass;

	/** Projectile initial speed (cm/sec). Used for leading and (if possible) applied to projectile movement. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret|Firing", meta = (ClampMin = "0.0"))
	float ProjectileSpeed = 6000.0f;

	/** Maximum random spread cone (degrees). Accuracy scales this down. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret|Firing", meta = (ClampMin = "0.0", ClampMax = "45.0"))
	float SpreadAngleDeg = 2.5f;

	/** 0..1. 1 = perfectly accurate (no spread). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret|Firing", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Accuracy = 0.85f;

	/** If true, only fires when within this angular error (degrees) of the desired aim direction. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret|Firing", meta = (ClampMin = "0.0", ClampMax = "90.0"))
	float FireAngleToleranceDeg = 3.0f;

	/** Manually start/stop firing (overrides bAutoFire when called). */
	UFUNCTION(BlueprintCallable, Category = "Turret|Firing")
	void SetFiringEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Turret|Firing")
	bool IsFiringEnabled() const { return bFiringEnabled; }

	// ============================================================================
	// DEBUG
	// ============================================================================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turret|Debug")
	bool bDrawDebug = false;

protected:
	void AcquireTarget();
	AActor* AcquireTargetFromEnemyAI() const;
	bool IsTargetValid(AActor* Candidate) const;
	bool HasLineOfSightTo(AActor* Candidate) const;

	void ResolveSetupComponents();
	void RefreshShipPartAim(AActor* Target, bool bForce);
	/** False for null/invalid parts and thrusters at 0 HP. */
	static bool IsShipPartViable(const USceneComponent* Part);

	FVector GetMuzzleLocation() const;
	/** Muzzle used for aiming/LOS when multiple muzzles exist (stable; does not alternate each tick). */
	USceneComponent* GetAimMuzzleSceneComponent() const;
	/** Muzzle used for the next shot (round-robin when enabled). */
	USceneComponent* GetFireMuzzleSceneComponent() const;
	FVector GetTargetAimPoint(AActor* Target) const;
	FVector GetActorFallbackAimPoint(AActor* Target) const;
	FVector ComputeLeadAimPoint(const FVector& MuzzleLoc, AActor* Target) const;
	static bool SolveInterceptTime(const FVector& RelativePos, const FVector& TargetVel, float ProjectileSpeedCmPerSec, float& OutT);

	void UpdateAim(float DeltaTime, const FVector& SmoothedAimWorld);
	bool IsAimedAt(const USceneComponent* Muzzle, const FVector& AimPoint) const;

	void TryFire(float DeltaTime, const FVector& SmoothedAimWorld);
	void FireOnce(const USceneComponent* SpawnFrom, const FVector& AimPoint);
	FVector ApplySpreadToDirection(const FVector& Direction) const;

private:
	/** Resolved components (cached at runtime). */
	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> ResolvedYawComponent = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> ResolvedPitchComponent = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> ResolvedMuzzleComponent = nullptr;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USceneComponent>> ResolvedMuzzleComponents;

	UPROPERTY()
	TWeakObjectPtr<AActor> CurrentTarget;

	UPROPERTY(Transient)
	TWeakObjectPtr<USceneComponent> CurrentAimPart;

	FRotator InitialYawRelative = FRotator::ZeroRotator;
	FRotator InitialPitchRelative = FRotator::ZeroRotator;

	float TimeSinceLastScan = 0.0f;
	float TimeSincePartReselect = 0.0f;
	float FireCooldown = 0.0f;
	bool bFiringEnabled = true;

	int32 NextMuzzleIndex = 0;
	FVector SmoothedAimPoint = FVector::ZeroVector;
	bool bHasSmoothedAimPoint = false;
};

