// Hover Thruster Component - Provides hover physics for hovercraft vehicles
// Handles lift, terrain adjustment, and damage states that affect thruster performance

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "HoverThrusterComponent.generated.h"

/**
 * Thruster health state - affects thruster behavior
 */
UENUM(BlueprintType)
enum class EThrusterHealthState : uint8
{
	Healthy			UMETA(DisplayName = "Healthy (100%)"),
	Damaged			UMETA(DisplayName = "Damaged (50-99%)"),
	Critical		UMETA(DisplayName = "Critical (25-49%)"),
	Failing			UMETA(DisplayName = "Failing (1-24%)"),
	Destroyed		UMETA(DisplayName = "Destroyed (0%)")
};

// Delegate declarations for thruster events
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnThrusterDamaged, float, CurrentHitpoints, float, DamageAmount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnThrusterHealed, float, CurrentHitpoints, float, HealAmount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnThrusterStateChanged, EThrusterHealthState, OldState, EThrusterHealthState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnThrusterDestroyed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnThrusterRepaired);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnThrusterSputter, float, SputterStrength);

/**
 * Hover Thruster Component
 *
 * A scene component that provides hover physics for hovercraft vehicles.
 * Attach multiple thrusters (typically 4) to your pawn for stable hovering.
 *
 * Features:
 * - Physics-based hover force with ground detection
 * - Terrain following and object avoidance
 * - Damage/hitpoint system that affects thruster performance
 * - Sputter and malfunction effects when damaged
 * - Full Blueprint exposure for all settings
 *
 * Usage:
 *   1. Add 4 UHoverThrusterComponent instances to your Pawn
 *   2. Position them at the corners of your vehicle
 *   3. Configure hover height, force, and damping
 *   4. Call ApplyHoverForce() in Tick or physics update
 */
UCLASS(ClassGroup=(Vehicle), meta=(BlueprintSpawnableComponent), BlueprintType, Blueprintable)
class UNDUINOCPP_API UHoverThrusterComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UHoverThrusterComponent();

	// === UActorComponent Interface ===
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ============================================================================
	// HOVER PHYSICS SETTINGS
	// ============================================================================

	/** Target hover height above ground (in cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Thruster|Physics", meta = (ClampMin = "0.0", ClampMax = "1000.0"))
	float HoverHeight = 150.0f;

	/** Maximum force the thruster can apply (in Newtons) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Thruster|Physics", meta = (ClampMin = "0.0"))
	float MaxHoverForce = 50000.0f;

	/** Spring stiffness for hover suspension - higher = stiffer response */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Thruster|Physics", meta = (ClampMin = "0.0"))
	float HoverStiffness = 5000.0f;

	/** Damping factor for hover oscillation - prevents bouncing */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Thruster|Physics", meta = (ClampMin = "0.0"))
	float HoverDamping = 1000.0f;

	/** Angular damping applied to stabilize rotation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Thruster|Physics", meta = (ClampMin = "0.0"))
	float AngularDamping = 2.0f;

	/** How far to trace for ground detection (multiplier of HoverHeight) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Thruster|Physics", meta = (ClampMin = "1.0", ClampMax = "10.0"))
	float TraceDistanceMultiplier = 2.0f;

	/** Trace channel for ground detection */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Thruster|Physics")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	/** If true, draw debug visualization of hover traces */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Thruster|Debug")
	bool bDrawDebug = false;

	// ============================================================================
	// HITPOINT / DAMAGE SYSTEM
	// ============================================================================

	/** Maximum hitpoints for this thruster */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Thruster|Health", meta = (ClampMin = "1.0"))
	float MaxHitpoints = 100.0f;

	/** Current hitpoints - affects thruster performance */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Thruster|Health", meta = (ClampMin = "0.0"), ReplicatedUsing = OnRep_CurrentHitpoints)
	float CurrentHitpoints = 100.0f;

	/** Whether this thruster can be damaged */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Thruster|Health")
	bool bCanBeDamaged = true;

	/** Whether this thruster auto-repairs over time when not destroyed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Thruster|Health")
	bool bAutoRepair = false;

	/** Hitpoints regenerated per second when auto-repair is enabled */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Thruster|Health", meta = (ClampMin = "0.0", EditCondition = "bAutoRepair"))
	float AutoRepairRate = 5.0f;

	// ============================================================================
	// DAMAGE STATE THRESHOLDS
	// ============================================================================

	/** Health percentage below which thruster is considered Damaged (default: 99%) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Thruster|Health|Thresholds", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float DamagedThreshold = 99.0f;

	/** Health percentage below which thruster is considered Critical (default: 50%) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Thruster|Health|Thresholds", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float CriticalThreshold = 50.0f;

	/** Health percentage below which thruster is considered Failing (default: 25%) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Thruster|Health|Thresholds", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float FailingThreshold = 25.0f;

	// ============================================================================
	// MALFUNCTION / SPUTTER SETTINGS
	// ============================================================================

	/** If true, damaged thrusters will sputter and cause instability */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Thruster|Malfunction")
	bool bEnableMalfunction = true;

	/** Base chance per second for sputter when in Damaged state (0-1) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Thruster|Malfunction", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DamagedSputterChance = 0.1f;

	/** Base chance per second for sputter when in Critical state (0-1) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Thruster|Malfunction", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CriticalSputterChance = 0.3f;

	/** Base chance per second for sputter when in Failing state (0-1) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Thruster|Malfunction", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FailingSputterChance = 0.6f;

	/** Duration of a sputter event in seconds */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Thruster|Malfunction", meta = (ClampMin = "0.01", ClampMax = "2.0"))
	float SputterDuration = 0.15f;

	/** Force multiplier during sputter (0 = no force, 1 = full force) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Thruster|Malfunction", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SputterForceMultiplier = 0.2f;

	// ============================================================================
	// EVENTS
	// ============================================================================

	/** Event fired when thruster takes damage */
	UPROPERTY(BlueprintAssignable, Category = "Hover Thruster|Events")
	FOnThrusterDamaged OnThrusterDamaged;

	/** Event fired when thruster is healed */
	UPROPERTY(BlueprintAssignable, Category = "Hover Thruster|Events")
	FOnThrusterHealed OnThrusterHealed;

	/** Event fired when thruster health state changes */
	UPROPERTY(BlueprintAssignable, Category = "Hover Thruster|Events")
	FOnThrusterStateChanged OnThrusterStateChanged;

	/** Event fired when thruster is destroyed (0 hitpoints) */
	UPROPERTY(BlueprintAssignable, Category = "Hover Thruster|Events")
	FOnThrusterDestroyed OnThrusterDestroyed;

	/** Event fired when thruster is fully repaired from destroyed state */
	UPROPERTY(BlueprintAssignable, Category = "Hover Thruster|Events")
	FOnThrusterRepaired OnThrusterRepaired;

	/** Event fired during a sputter, with strength 0-1 indicating severity */
	UPROPERTY(BlueprintAssignable, Category = "Hover Thruster|Events")
	FOnThrusterSputter OnThrusterSputter;

	// ============================================================================
	// BLUEPRINT CALLABLE FUNCTIONS
	// ============================================================================

	/**
	 * Apply damage to the thruster
	 * @param DamageAmount - Amount of damage to apply
	 * @return Remaining hitpoints after damage
	 */
	UFUNCTION(BlueprintCallable, Category = "Hover Thruster|Health")
	float ApplyDamage(float DamageAmount);

	/**
	 * Heal the thruster
	 * @param HealAmount - Amount of hitpoints to restore
	 * @return Current hitpoints after healing
	 */
	UFUNCTION(BlueprintCallable, Category = "Hover Thruster|Health")
	float Heal(float HealAmount);

	/**
	 * Fully repair the thruster to max hitpoints
	 */
	UFUNCTION(BlueprintCallable, Category = "Hover Thruster|Health")
	void FullRepair();

	/**
	 * Set hitpoints directly (clamped to 0 - MaxHitpoints)
	 * @param NewHitpoints - New hitpoint value
	 */
	UFUNCTION(BlueprintCallable, Category = "Hover Thruster|Health")
	void SetHitpoints(float NewHitpoints);

	/**
	 * Get current health percentage (0-100)
	 * @return Health as percentage
	 */
	UFUNCTION(BlueprintPure, Category = "Hover Thruster|Health")
	float GetHealthPercent() const;

	/**
	 * Get current thruster health state
	 * @return Current health state enum
	 */
	UFUNCTION(BlueprintPure, Category = "Hover Thruster|Health")
	EThrusterHealthState GetHealthState() const;

	/**
	 * Check if thruster is destroyed (0 hitpoints)
	 * @return True if destroyed
	 */
	UFUNCTION(BlueprintPure, Category = "Hover Thruster|Health")
	bool IsDestroyed() const;

	/**
	 * Check if thruster is currently sputtering
	 * @return True if in sputter state
	 */
	UFUNCTION(BlueprintPure, Category = "Hover Thruster|Health")
	bool IsSputtering() const;

	/**
	 * Get the current force effectiveness (0-1) based on health and sputter state
	 * @return Force multiplier accounting for damage and malfunction
	 */
	UFUNCTION(BlueprintPure, Category = "Hover Thruster|Physics")
	float GetForceEffectiveness() const;

	/**
	 * Perform ground trace and calculate hover force
	 * Call this in your physics update or Tick
	 * @param DeltaTime - Time since last update
	 * @return The hover force vector to apply
	 */
	UFUNCTION(BlueprintCallable, Category = "Hover Thruster|Physics")
	FVector CalculateHoverForce(float DeltaTime);

	/**
	 * Apply hover force to the owning actor's root primitive component
	 * @param DeltaTime - Time since last update
	 * @return True if force was applied
	 */
	UFUNCTION(BlueprintCallable, Category = "Hover Thruster|Physics")
	bool ApplyHoverForce(float DeltaTime);

	/**
	 * Check if the thruster is currently detecting ground
	 * @return True if ground is within trace distance
	 */
	UFUNCTION(BlueprintPure, Category = "Hover Thruster|Physics")
	bool IsGroundDetected() const;

	/**
	 * Get the current distance to ground
	 * @return Distance in cm, or -1 if no ground detected
	 */
	UFUNCTION(BlueprintPure, Category = "Hover Thruster|Physics")
	float GetDistanceToGround() const;

	/**
	 * Get the surface normal at ground contact point
	 * @return Surface normal, or Up vector if no ground
	 */
	UFUNCTION(BlueprintPure, Category = "Hover Thruster|Physics")
	FVector GetGroundNormal() const;

	/**
	 * Enable or disable this thruster (bypasses damage system)
	 * @param bNewEnabled - Whether thruster should be enabled
	 */
	UFUNCTION(BlueprintCallable, Category = "Hover Thruster")
	void SetThrusterEnabled(bool bNewEnabled);

	/**
	 * Check if thruster is enabled
	 * @return True if enabled
	 */
	UFUNCTION(BlueprintPure, Category = "Hover Thruster")
	bool IsThrusterEnabled() const;

	// Replication support
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	/** Handle hitpoints replication */
	UFUNCTION()
	void OnRep_CurrentHitpoints();

	/** Update health state and fire events if changed */
	void UpdateHealthState();

	/** Process auto-repair if enabled */
	void ProcessAutoRepair(float DeltaTime);

	/** Roll for sputter based on current health state */
	void ProcessMalfunction(float DeltaTime);

	/** Perform the ground trace */
	bool PerformGroundTrace(FHitResult& OutHit) const;

	/** Calculate the base force effectiveness from health percentage */
	float GetHealthForceMultiplier() const;

private:
	/** Whether this thruster is manually enabled */
	bool bIsEnabled = true;

	/** Current health state */
	EThrusterHealthState CurrentHealthState = EThrusterHealthState::Healthy;

	/** Whether we are currently in a sputter */
	bool bIsSputtering = false;

	/** Time remaining in current sputter */
	float SputterTimeRemaining = 0.0f;

	/** Cached last ground trace result */
	bool bLastTraceHit = false;
	float LastDistanceToGround = -1.0f;
	FVector LastGroundNormal = FVector::UpVector;

	/** Previous hitpoints for detecting changes during replication */
	float PreviousHitpoints = 100.0f;

	/** Previous vertical velocity for damping calculation */
	float PreviousVerticalVelocity = 0.0f;
};
