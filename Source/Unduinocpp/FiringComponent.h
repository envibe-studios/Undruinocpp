// Firing Component - Weapon system for hovercraft vehicles
// Supports multiple firing modes: bullets, tractor beam, and scanner

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "FiringComponent.generated.h"

// Forward declarations
class UPrimitiveComponent;

/**
 * Firing mode type enumeration
 */
UENUM(BlueprintType)
enum class EFiringModeType : uint8
{
	Bullet			UMETA(DisplayName = "Bullet"),
	TractorBeam		UMETA(DisplayName = "Tractor Beam"),
	Scanner			UMETA(DisplayName = "Scanner"),
	Custom			UMETA(DisplayName = "Custom")
};

/**
 * Base firing mode configuration
 * Extend this struct for new firing modes
 */
USTRUCT(BlueprintType)
struct FFiringModeConfig
{
	GENERATED_BODY()

	/** Display name for this firing mode */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Firing Mode")
	FName ModeName = NAME_None;

	/** Type of firing mode */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Firing Mode")
	EFiringModeType ModeType = EFiringModeType::Bullet;

	/** Whether this mode is enabled */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Firing Mode")
	bool bEnabled = true;

	/** Range of this firing mode (in cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Firing Mode", meta = (ClampMin = "0.0"))
	float Range = 5000.0f;

	/** Trace channel for hit detection */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Firing Mode")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;
};

/**
 * Bullet firing mode configuration
 */
USTRUCT(BlueprintType)
struct FBulletModeConfig : public FFiringModeConfig
{
	GENERATED_BODY()

	FBulletModeConfig()
	{
		ModeName = FName("Bullet");
		ModeType = EFiringModeType::Bullet;
	}

	/** Damage per bullet hit */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet Mode", meta = (ClampMin = "0.0"))
	float Damage = 25.0f;

	/** Rounds fired per second */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet Mode", meta = (ClampMin = "0.1"))
	float RateOfFire = 10.0f;

	/** Bullet spread angle in degrees (0 = perfectly accurate) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet Mode", meta = (ClampMin = "0.0", ClampMax = "45.0"))
	float SpreadAngle = 1.0f;

	/** Number of bullets per shot (for shotgun-style weapons) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet Mode", meta = (ClampMin = "1"))
	int32 BulletsPerShot = 1;

	/** Whether to use ammo */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet Mode")
	bool bUseAmmo = false;

	/** Current ammo count (if using ammo) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet Mode", meta = (ClampMin = "0", EditCondition = "bUseAmmo"))
	int32 CurrentAmmo = 100;

	/** Maximum ammo capacity */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet Mode", meta = (ClampMin = "1", EditCondition = "bUseAmmo"))
	int32 MaxAmmo = 100;
};

/**
 * Tractor beam mode configuration
 */
USTRUCT(BlueprintType)
struct FTractorBeamModeConfig : public FFiringModeConfig
{
	GENERATED_BODY()

	FTractorBeamModeConfig()
	{
		ModeName = FName("TractorBeam");
		ModeType = EFiringModeType::TractorBeam;
		Range = 3000.0f;
	}

	/** Force applied to pull objects (in Newtons) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tractor Beam", meta = (ClampMin = "0.0"))
	float PullForce = 50000.0f;

	/** Distance at which object is considered "collected" (in cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tractor Beam", meta = (ClampMin = "1.0"))
	float CollectionDistance = 100.0f;

	/** Rate at which captured object shrinks (scale per second) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tractor Beam", meta = (ClampMin = "0.0"))
	float ShrinkRate = 2.0f;

	/** Minimum scale before object is collected */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tractor Beam", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float MinScaleForCollection = 0.1f;

	/** Tag that objects must have to be tractored (empty = all physics objects) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tractor Beam")
	FName TractorableTag = NAME_None;

	/** Maximum mass of objects that can be tractored (0 = unlimited) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tractor Beam", meta = (ClampMin = "0.0"))
	float MaxMass = 0.0f;
};

/**
 * Scanner mode configuration
 */
USTRUCT(BlueprintType)
struct FScannerModeConfig : public FFiringModeConfig
{
	GENERATED_BODY()

	FScannerModeConfig()
	{
		ModeName = FName("Scanner");
		ModeType = EFiringModeType::Scanner;
		Range = 5000.0f;
	}

	/** Time required to complete a scan (in seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scanner", meta = (ClampMin = "0.1"))
	float ScanDuration = 2.0f;

	/** Tag that objects must have to be scannable (empty = all objects) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scanner")
	FName ScannableTag = NAME_None;

	/** Angle of the scanner cone (in degrees) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scanner", meta = (ClampMin = "0.0", ClampMax = "90.0"))
	float ScanConeAngle = 5.0f;

	/** Whether the target must stay in beam for entire scan duration */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scanner")
	bool bRequireContinuousLock = true;

	/** Time before scan progress resets when target is lost (only if bRequireContinuousLock is false) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scanner", meta = (ClampMin = "0.0", EditCondition = "!bRequireContinuousLock"))
	float ScanResetDelay = 1.0f;
};

// ============================================================================
// DELEGATE DECLARATIONS
// ============================================================================

// Bullet mode delegates
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnBulletFired, FVector, Origin, FVector, Direction, float, Damage, int32, BulletIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FOnBulletHit, AActor*, HitActor, FVector, HitLocation, FVector, HitNormal, float, DamageDealt, UPrimitiveComponent*, HitComponent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAmmoEmpty);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAmmoChanged, int32, CurrentAmmo, int32, MaxAmmo);

// Tractor beam delegates
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTractorBeamStart, AActor*, TargetActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTractorBeamPulling, AActor*, TargetActor, float, DistanceRemaining);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnObjectCollected, AActor*, CollectedActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTractorBeamLost, AActor*, LostActor);

// Scanner delegates
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnScanStart, AActor*, TargetActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnScanning, AActor*, TargetActor, float, Progress, float, TimeRemaining);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnScanComplete, AActor*, ScannedActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnScanCancelled, AActor*, TargetActor, float, Progress);

// General firing delegates
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFiringModeChanged, EFiringModeType, NewMode);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFiringStarted, EFiringModeType, Mode);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFiringStopped, EFiringModeType, Mode);

/**
 * Firing Component
 *
 * A versatile weapon system component for hover vehicles supporting multiple firing modes.
 * Each mode has distinct behavior and can be extended through the config structs.
 *
 * Features:
 * - Bullet mode: High rate of fire projectile weapon with damage
 * - Tractor beam: Pulls objects toward the gun, shrinks and collects them
 * - Scanner: Scans objects over time with progress events
 * - Extensible architecture for custom firing modes
 *
 * Usage:
 *   1. Add UFiringComponent to your vehicle pawn
 *   2. Configure firing mode settings
 *   3. Call SetFiring(true) to start firing
 *   4. Bind to delegates for feedback and game logic
 */
UCLASS(ClassGroup=(Weapon), meta=(BlueprintSpawnableComponent), BlueprintType, Blueprintable)
class UNDUINOCPP_API UFiringComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UFiringComponent();

	// === UActorComponent Interface ===
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ============================================================================
	// FIRING MODE CONFIGURATIONS
	// ============================================================================

	/** Current active firing mode type */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Firing|Mode")
	EFiringModeType CurrentFiringMode = EFiringModeType::Bullet;

	/** Bullet mode configuration */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Firing|Bullet Mode")
	FBulletModeConfig BulletConfig;

	/** Tractor beam mode configuration */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Firing|Tractor Beam Mode")
	FTractorBeamModeConfig TractorBeamConfig;

	/** Scanner mode configuration */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Firing|Scanner Mode")
	FScannerModeConfig ScannerConfig;

	// ============================================================================
	// DEBUG
	// ============================================================================

	/** Draw debug visualization */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Firing|Debug")
	bool bDrawDebug = false;

	// ============================================================================
	// BULLET MODE EVENTS
	// ============================================================================

	/** Fired when a bullet is shot */
	UPROPERTY(BlueprintAssignable, Category = "Firing|Events|Bullet")
	FOnBulletFired OnBulletFired;

	/** Fired when a bullet hits something */
	UPROPERTY(BlueprintAssignable, Category = "Firing|Events|Bullet")
	FOnBulletHit OnBulletHit;

	/** Fired when ammo is depleted */
	UPROPERTY(BlueprintAssignable, Category = "Firing|Events|Bullet")
	FOnAmmoEmpty OnAmmoEmpty;

	/** Fired when ammo count changes */
	UPROPERTY(BlueprintAssignable, Category = "Firing|Events|Bullet")
	FOnAmmoChanged OnAmmoChanged;

	// ============================================================================
	// TRACTOR BEAM EVENTS
	// ============================================================================

	/** Fired when tractor beam locks onto a target */
	UPROPERTY(BlueprintAssignable, Category = "Firing|Events|Tractor Beam")
	FOnTractorBeamStart OnTractorBeamStart;

	/** Fired each tick while pulling an object */
	UPROPERTY(BlueprintAssignable, Category = "Firing|Events|Tractor Beam")
	FOnTractorBeamPulling OnTractorBeamPulling;

	/** Fired when an object is fully collected */
	UPROPERTY(BlueprintAssignable, Category = "Firing|Events|Tractor Beam")
	FOnObjectCollected OnObjectCollected;

	/** Fired when tractor beam loses its target */
	UPROPERTY(BlueprintAssignable, Category = "Firing|Events|Tractor Beam")
	FOnTractorBeamLost OnTractorBeamLost;

	// ============================================================================
	// SCANNER EVENTS
	// ============================================================================

	/** Fired when scan begins on a target */
	UPROPERTY(BlueprintAssignable, Category = "Firing|Events|Scanner")
	FOnScanStart OnScanStart;

	/** Fired each tick during scanning with progress */
	UPROPERTY(BlueprintAssignable, Category = "Firing|Events|Scanner")
	FOnScanning OnScanning;

	/** Fired when scan is complete */
	UPROPERTY(BlueprintAssignable, Category = "Firing|Events|Scanner")
	FOnScanComplete OnScanComplete;

	/** Fired when scan is interrupted */
	UPROPERTY(BlueprintAssignable, Category = "Firing|Events|Scanner")
	FOnScanCancelled OnScanCancelled;

	// ============================================================================
	// GENERAL EVENTS
	// ============================================================================

	/** Fired when firing mode changes */
	UPROPERTY(BlueprintAssignable, Category = "Firing|Events")
	FOnFiringModeChanged OnFiringModeChanged;

	/** Fired when firing starts */
	UPROPERTY(BlueprintAssignable, Category = "Firing|Events")
	FOnFiringStarted OnFiringStarted;

	/** Fired when firing stops */
	UPROPERTY(BlueprintAssignable, Category = "Firing|Events")
	FOnFiringStopped OnFiringStopped;

	// ============================================================================
	// CONTROL FUNCTIONS
	// ============================================================================

	/**
	 * Set whether the weapon is firing
	 * @param bShouldFire - True to start firing, false to stop
	 */
	UFUNCTION(BlueprintCallable, Category = "Firing|Control")
	void SetFiring(bool bShouldFire);

	/**
	 * Check if currently firing
	 * @return True if firing
	 */
	UFUNCTION(BlueprintPure, Category = "Firing|Control")
	bool IsFiring() const;

	/**
	 * Change the current firing mode
	 * @param NewMode - The firing mode to switch to
	 */
	UFUNCTION(BlueprintCallable, Category = "Firing|Control")
	void SetFiringMode(EFiringModeType NewMode);

	/**
	 * Get the current firing mode
	 * @return Current firing mode type
	 */
	UFUNCTION(BlueprintPure, Category = "Firing|Control")
	EFiringModeType GetFiringMode() const;

	/**
	 * Cycle to the next available firing mode
	 */
	UFUNCTION(BlueprintCallable, Category = "Firing|Control")
	void CycleNextFiringMode();

	/**
	 * Cycle to the previous firing mode
	 */
	UFUNCTION(BlueprintCallable, Category = "Firing|Control")
	void CyclePreviousFiringMode();

	// ============================================================================
	// BULLET MODE FUNCTIONS
	// ============================================================================

	/**
	 * Add ammo to the bullet mode
	 * @param Amount - Amount of ammo to add
	 * @return New ammo count
	 */
	UFUNCTION(BlueprintCallable, Category = "Firing|Bullet")
	int32 AddAmmo(int32 Amount);

	/**
	 * Set ammo directly
	 * @param Amount - New ammo count
	 */
	UFUNCTION(BlueprintCallable, Category = "Firing|Bullet")
	void SetAmmo(int32 Amount);

	/**
	 * Get current ammo count
	 * @return Current ammo
	 */
	UFUNCTION(BlueprintPure, Category = "Firing|Bullet")
	int32 GetCurrentAmmo() const;

	/**
	 * Get maximum ammo capacity
	 * @return Max ammo
	 */
	UFUNCTION(BlueprintPure, Category = "Firing|Bullet")
	int32 GetMaxAmmo() const;

	// ============================================================================
	// TRACTOR BEAM FUNCTIONS
	// ============================================================================

	/**
	 * Check if tractor beam has a target
	 * @return True if currently tractoring an object
	 */
	UFUNCTION(BlueprintPure, Category = "Firing|Tractor Beam")
	bool HasTractorTarget() const;

	/**
	 * Get the current tractor beam target
	 * @return The actor being tractored, or nullptr
	 */
	UFUNCTION(BlueprintPure, Category = "Firing|Tractor Beam")
	AActor* GetTractorTarget() const;

	/**
	 * Release the current tractor target
	 */
	UFUNCTION(BlueprintCallable, Category = "Firing|Tractor Beam")
	void ReleaseTractorTarget();

	// ============================================================================
	// SCANNER FUNCTIONS
	// ============================================================================

	/**
	 * Check if scanner has a target
	 * @return True if currently scanning
	 */
	UFUNCTION(BlueprintPure, Category = "Firing|Scanner")
	bool HasScanTarget() const;

	/**
	 * Get the current scan target
	 * @return The actor being scanned, or nullptr
	 */
	UFUNCTION(BlueprintPure, Category = "Firing|Scanner")
	AActor* GetScanTarget() const;

	/**
	 * Get current scan progress (0-1)
	 * @return Scan progress percentage
	 */
	UFUNCTION(BlueprintPure, Category = "Firing|Scanner")
	float GetScanProgress() const;

	/**
	 * Cancel the current scan
	 */
	UFUNCTION(BlueprintCallable, Category = "Firing|Scanner")
	void CancelScan();

	// ============================================================================
	// IMU ORIENTATION / ZEROING
	// ============================================================================

	/**
	 * Zero the weapon orientation.
	 * Captures the current raw IMU quaternion as the "zero reference".
	 * All subsequent IMU updates will produce rotation deltas relative to this pose.
	 * If no IMU data has been received yet, the next incoming quaternion will be used as zero.
	 */
	UFUNCTION(BlueprintCallable, Category = "Firing|IMU")
	void ZeroOrientation();

	/**
	 * Apply a raw IMU quaternion to this component's relative rotation.
	 * The rotation is computed as the delta from the zero-reference quaternion,
	 * so the weapon points "forward" at the zeroed pose and tracks movement from there.
	 * If ZeroOrientation has not been called yet, the first quaternion received is
	 * automatically used as the zero reference.
	 * @param RawImuQuat - The raw quaternion from the 9DOF sensor
	 */
	UFUNCTION(BlueprintCallable, Category = "Firing|IMU")
	void ApplyImuOrientation(const FQuat& RawImuQuat);

	/**
	 * Check whether the orientation has been zeroed
	 * @return True if ZeroOrientation has been called (or auto-zeroed on first IMU data)
	 */
	UFUNCTION(BlueprintPure, Category = "Firing|IMU")
	bool IsOrientationZeroed() const;

	// ============================================================================
	// WEAPON MAG INTEGRATION
	// ============================================================================

	/**
	 * Apply weapon magazine configuration to this firing component
	 * Updates firing mode, damage, rate of fire, spread, bullets per shot, max ammo, and range
	 * @param bActive - Whether the weapon mag is active/enabled
	 * @param FiringMode - The firing mode type (0=Bullet, 1=TractorBeam, 2=Scanner)
	 * @param Damage - Bullet damage
	 * @param RateOfFire - Rounds per second
	 * @param SpreadAngle - Bullet spread in degrees
	 * @param BulletsPerShot - Bullets per trigger pull
	 * @param MaxAmmo - Maximum ammo capacity
	 * @param CurrentAmmo - Current ammo in the mag (if -1, uses MaxAmmo)
	 * @param Range - Firing range in cm
	 * @param TractorPullForce - Pull force for tractor beam mode
	 * @param ScanDuration - Scan duration for scanner mode
	 */
	UFUNCTION(BlueprintCallable, Category = "Firing|Weapon Mag")
	void ApplyWeaponMagConfig(
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
	);

protected:
	// ============================================================================
	// INTERNAL PROCESSING
	// ============================================================================

	/** Process bullet firing mode */
	void ProcessBulletMode(float DeltaTime);

	/** Fire a single bullet (or burst) */
	void FireBullet();

	/** Process tractor beam mode */
	void ProcessTractorBeamMode(float DeltaTime);

	/** Find a valid tractor target */
	AActor* FindTractorTarget();

	/** Check if an actor can be tractored */
	bool CanTractorActor(AActor* Actor) const;

	/** Process scanner mode */
	void ProcessScannerMode(float DeltaTime);

	/** Find a valid scan target */
	AActor* FindScanTarget();

	/** Check if an actor can be scanned */
	bool CanScanActor(AActor* Actor) const;

	/** Perform a line trace from the component */
	bool PerformTrace(FHitResult& OutHit, float Range, ECollisionChannel Channel) const;

	/** Get the firing direction (forward vector of component) */
	FVector GetFiringDirection() const;

	/** Get the firing origin (component location) */
	FVector GetFiringOrigin() const;

	/** Apply spread to a direction vector */
	FVector ApplySpread(const FVector& Direction, float SpreadAngle) const;

	/** Reset mode-specific state when switching modes */
	void ResetModeState();

private:
	/** Whether weapon is actively firing */
	bool bIsFiring = false;

	/** Time until next bullet can be fired */
	float BulletCooldown = 0.0f;

	/** Current tractor beam target */
	UPROPERTY()
	TWeakObjectPtr<AActor> TractorTarget;

	/** Original scale of tractored object (for restoration if released) */
	FVector TractorTargetOriginalScale = FVector::OneVector;

	/** Current scan target */
	UPROPERTY()
	TWeakObjectPtr<AActor> ScanTarget;

	/** Current scan progress (0-1) */
	float CurrentScanProgress = 0.0f;

	/** Time since scan target was lost (for delayed reset) */
	float ScanLostTime = 0.0f;

	// === IMU Zeroing State ===

	/** Whether the zero-reference quaternion has been captured */
	bool bHasZeroReference = false;

	/** The inverse of the IMU quaternion captured at zero time.
	 *  Delta = ZeroInverse * CurrentRaw gives the rotation relative to zero pose. */
	FQuat ZeroInverseQuat = FQuat::Identity;
};
