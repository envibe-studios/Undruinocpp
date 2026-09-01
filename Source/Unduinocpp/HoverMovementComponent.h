// Hover Movement Component - Provides movement controls for hover vehicles
// Handles forward/backward thrust, turning, boost/turbo, and coordinates with hover thrusters

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HoverMovementComponent.generated.h"

// Forward declaration
class UHoverThrusterComponent;

// Delegate for movement input changes
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMovementInputChanged, float, ThrottleInput, float, SteeringInput);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnThrottleChanged, float, NewThrottle);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSteeringChanged, float, NewSteering);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBoostActiveChanged, bool, bIsBoostActive);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBoostEnergyChanged, float, BoostEnergy);

/**
 * Hover Movement Component
 *
 * Provides movement controls for hover vehicles with physics-based thrust and turning.
 * Designed to work with UHoverThrusterComponent for complete hover vehicle control.
 *
 * Features:
 * - Forward/backward thrust with gradual input support (pedals, triggers)
 * - Left/right turning with gradual input support (steering wheels, joysticks)
 * - Turn-bank via differential thruster hover-height bias (banks into turns with real hover physics)
 * - Configurable acceleration and deceleration curves
 * - Input smoothing for natural feel
 * - Hold-to-boost turbo with draining/recharging energy pool
 * - Full Blueprint exposure for input binding
 *
 * Usage:
 *   1. Add UHoverMovementComponent to your hover vehicle pawn
 *   2. Configure thrust and turn forces
 *   3. Bind SetThrottleInput() and SetSteeringInput() to input actions
 *   4. For digital input, use MoveForward/MoveBackward/TurnLeft/TurnRight
 *   5. Tune Hover Movement|Turn Bank on the movement component (works with registered thrusters)
 *   6. Bind SetBoostInput() for keyboard or ESP32 turbo start/end
 */
UCLASS(ClassGroup=(Vehicle), meta=(BlueprintSpawnableComponent), BlueprintType, Blueprintable)
class UNDUINOCPP_API UHoverMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHoverMovementComponent();

	// === UActorComponent Interface ===
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ============================================================================
	// THRUST SETTINGS
	// ============================================================================

	/** Maximum forward thrust force (in Newtons) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Movement|Thrust", meta = (ClampMin = "0.0"))
	float MaxForwardThrust = 30000.0f;

	/** Maximum backward thrust force (in Newtons) - typically less than forward */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Movement|Thrust", meta = (ClampMin = "0.0"))
	float MaxBackwardThrust = 15000.0f;

	/** How quickly thrust builds up when input is applied (units per second) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Movement|Thrust", meta = (ClampMin = "0.1"))
	float ThrustAcceleration = 3.0f;

	/** How quickly thrust decreases when input is released (units per second) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Movement|Thrust", meta = (ClampMin = "0.1"))
	float ThrustDeceleration = 5.0f;

	/** Linear drag applied to forward/backward movement */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Movement|Thrust", meta = (ClampMin = "0.0"))
	float LinearDrag = 0.5f;

	/** Height offset for thrust application point (relative to actor origin) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Movement|Thrust")
	float ThrustHeightOffset = 0.0f;

	// ============================================================================
	// TURNING SETTINGS
	// ============================================================================

	/** Maximum turning torque (in Newton-meters) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Movement|Turning", meta = (ClampMin = "0.0"))
	float MaxTurnTorque = 50000.0f;

	/** How quickly steering builds up when input is applied (units per second) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Movement|Turning", meta = (ClampMin = "0.1"))
	float SteeringAcceleration = 5.0f;

	/** How quickly steering returns to center when input is released (units per second) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Movement|Turning", meta = (ClampMin = "0.1"))
	float SteeringDeceleration = 8.0f;

	/** Angular drag applied to turning - higher values make turns feel tighter */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Movement|Turning", meta = (ClampMin = "0.0"))
	float AngularDrag = 2.0f;

	/** If true, turn rate scales with forward speed (more realistic) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Movement|Turning")
	bool bSpeedAffectsTurning = true;

	/** Minimum turn multiplier when stationary (0-1) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Movement|Turning", meta = (ClampMin = "0.0", ClampMax = "1.0", EditCondition = "bSpeedAffectsTurning"))
	float MinTurnMultiplierAtRest = 0.3f;

	/** Speed at which full turn rate is achieved (cm/s) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Movement|Turning", meta = (ClampMin = "1.0", EditCondition = "bSpeedAffectsTurning"))
	float FullTurnSpeed = 500.0f;

	// ============================================================================
	// TURN BANK (thruster-driven lean)
	// ============================================================================

	/**
	 * If true, steering biases outer/inner thruster hover heights so the craft banks into turns.
	 * Works with UHoverThrusterComponent springs instead of fighting angular damping with roll torque.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Movement|Turn Bank")
	bool bEnableTurnBank = true;

	/**
	 * Max hover-height bias applied to left/right thrusters at full bank (cm).
	 * Outer thrusters raise, inner thrusters lower, producing a physics bank into the turn.
	 * Start small (8-20) for a subtle hovercraft lean.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Movement|Turn Bank", meta = (ClampMin = "0.0", ClampMax = "100.0", EditCondition = "bEnableTurnBank"))
	float MaxBankHeightOffset = 14.0f;

	/** How quickly bank bias approaches the steering target */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Movement|Turn Bank", meta = (ClampMin = "0.1", EditCondition = "bEnableTurnBank"))
	float BankResponseSpeed = 4.0f;

	/** If true, bank amount scales with forward speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Movement|Turn Bank", meta = (EditCondition = "bEnableTurnBank"))
	bool bScaleBankWithSpeed = true;

	/** Minimum bank multiplier when nearly stationary (0-1) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Movement|Turn Bank", meta = (ClampMin = "0.0", ClampMax = "1.0", EditCondition = "bEnableTurnBank && bScaleBankWithSpeed"))
	float MinBankMultiplierAtRest = 0.15f;

	/** Forward speed at which full bank is available (cm/s) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Movement|Turn Bank", meta = (ClampMin = "1.0", EditCondition = "bEnableTurnBank && bScaleBankWithSpeed"))
	float FullBankSpeed = 600.0f;

	/**
	 * Optional assist torque around the forward axis to help the bank settle.
	 * Keep low; primary bank comes from thruster height bias. 0 = thrusters only.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Movement|Turn Bank", meta = (ClampMin = "0.0", EditCondition = "bEnableTurnBank"))
	float BankAssistTorque = 0.0f;

	// ============================================================================
	// INPUT SMOOTHING
	// ============================================================================

	/** If true, apply smoothing to input values */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Movement|Input")
	bool bSmoothInput = true;

	/** Smoothing factor for throttle input (0 = no smoothing, higher = more smoothing) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Movement|Input", meta = (ClampMin = "0.0", ClampMax = "20.0", EditCondition = "bSmoothInput"))
	float ThrottleSmoothingSpeed = 4.0f;

	/** Smoothing factor for steering input (0 = no smoothing, higher = more smoothing) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Movement|Input", meta = (ClampMin = "0.0", ClampMax = "20.0", EditCondition = "bSmoothInput"))
	float SteeringSmoothingSpeed = 6.0f;

	// ============================================================================
	// STRAFE SETTINGS (Optional lateral movement)
	// ============================================================================

	/** If true, enable strafing (lateral movement) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Movement|Strafe")
	bool bEnableStrafe = false;

	/** Maximum strafe thrust force (in Newtons) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Movement|Strafe", meta = (ClampMin = "0.0", EditCondition = "bEnableStrafe"))
	float MaxStrafeThrust = 15000.0f;

	// ============================================================================
	// BOOST / TURBO
	// ============================================================================

	/** If true, boost input and energy drain/recharge are processed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Movement|Boost")
	bool bEnableBoost = true;

	/** Maximum boost energy (seconds of continuous boost at BoostDrainRate 1.0) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Movement|Boost", meta = (ClampMin = "0.1", EditCondition = "bEnableBoost"))
	float MaxBoostEnergy = 3.0f;

	/** Energy drained per second while boost is active */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Movement|Boost", meta = (ClampMin = "0.0", EditCondition = "bEnableBoost"))
	float BoostDrainRate = 1.0f;

	/** Energy restored per second while boost is not active */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Movement|Boost", meta = (ClampMin = "0.0", EditCondition = "bEnableBoost"))
	float BoostRechargeRate = 0.4f;

	/** Minimum energy required to start boost after it has ended */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Movement|Boost", meta = (ClampMin = "0.0", EditCondition = "bEnableBoost"))
	float MinEnergyToStart = 0.2f;

	/** Forward thrust multiplier while boost is active */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Movement|Boost", meta = (ClampMin = "1.0", EditCondition = "bEnableBoost"))
	float BoostThrustMultiplier = 2.0f;

	// ============================================================================
	// DEBUG
	// ============================================================================

	/** If true, draw debug visualization of forces */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Movement|Debug")
	bool bDrawDebug = false;

	// ============================================================================
	// EVENTS
	// ============================================================================

	/** Event fired when movement input changes */
	UPROPERTY(BlueprintAssignable, Category = "Hover Movement|Events")
	FOnMovementInputChanged OnMovementInputChanged;

	/** Event fired when throttle changes */
	UPROPERTY(BlueprintAssignable, Category = "Hover Movement|Events")
	FOnThrottleChanged OnThrottleChanged;

	/** Event fired when steering changes */
	UPROPERTY(BlueprintAssignable, Category = "Hover Movement|Events")
	FOnSteeringChanged OnSteeringChanged;

	/** Event fired when boost active state changes */
	UPROPERTY(BlueprintAssignable, Category = "Hover Movement|Events")
	FOnBoostActiveChanged OnBoostActiveChanged;

	/** Event fired when boost energy changes */
	UPROPERTY(BlueprintAssignable, Category = "Hover Movement|Events")
	FOnBoostEnergyChanged OnBoostEnergyChanged;

	// ============================================================================
	// INPUT FUNCTIONS - Analog/Gradual Input (Triggers, Pedals, Steering Wheels)
	// ============================================================================

	/**
	 * Set throttle input for forward/backward movement
	 * Designed for analog input like triggers, pedals, or joystick axes
	 * @param Value - Input value from -1 (full reverse) to +1 (full forward)
	 */
	UFUNCTION(BlueprintCallable, Category = "Hover Movement|Input")
	void SetThrottleInput(float Value);

	/**
	 * Set steering input for turning
	 * Designed for analog input like steering wheels or joystick axes
	 * @param Value - Input value from -1 (full left) to +1 (full right)
	 */
	UFUNCTION(BlueprintCallable, Category = "Hover Movement|Input")
	void SetSteeringInput(float Value);

	/**
	 * Set strafe input for lateral movement (if enabled)
	 * @param Value - Input value from -1 (full left) to +1 (full right)
	 */
	UFUNCTION(BlueprintCallable, Category = "Hover Movement|Input")
	void SetStrafeInput(float Value);

	/**
	 * Authoritative analog drive for a full frame (dual sticks, etc.).
	 * When bActive is true, these values replace RawThrottle/Steering/Strafe at the start of
	 * movement Tick — so later SetThrottleInput(0) from ESP/keyboard idle cannot wipe them
	 * before forces are applied. When bActive is false, normal Set*Input / digital paths resume.
	 */
	UFUNCTION(BlueprintCallable, Category = "Hover Movement|Input")
	void SetExternalAnalogInput(float Throttle, float Steering, float Strafe, bool bActive);

	// ============================================================================
	// INPUT FUNCTIONS - Digital Input (Keyboard, Buttons)
	// ============================================================================

	/**
	 * Apply forward thrust (for digital input like keyboard)
	 * Call while key is held, throttle will smoothly increase
	 * @param bPressed - True while input is active
	 */
	UFUNCTION(BlueprintCallable, Category = "Hover Movement|Input")
	void MoveForward(bool bPressed);

	/**
	 * Apply backward thrust (for digital input like keyboard)
	 * Call while key is held, throttle will smoothly increase
	 * @param bPressed - True while input is active
	 */
	UFUNCTION(BlueprintCallable, Category = "Hover Movement|Input")
	void MoveBackward(bool bPressed);

	/**
	 * Apply left turn (for digital input like keyboard)
	 * Call while key is held, steering will smoothly increase
	 * @param bPressed - True while input is active
	 */
	UFUNCTION(BlueprintCallable, Category = "Hover Movement|Input")
	void TurnLeft(bool bPressed);

	/**
	 * Apply right turn (for digital input like keyboard)
	 * Call while key is held, steering will smoothly increase
	 * @param bPressed - True while input is active
	 */
	UFUNCTION(BlueprintCallable, Category = "Hover Movement|Input")
	void TurnRight(bool bPressed);

	/**
	 * Strafe left (for digital input, if strafe enabled)
	 * @param bPressed - True while input is active
	 */
	UFUNCTION(BlueprintCallable, Category = "Hover Movement|Input")
	void StrafeLeft(bool bPressed);

	/**
	 * Strafe right (for digital input, if strafe enabled)
	 * @param bPressed - True while input is active
	 */
	UFUNCTION(BlueprintCallable, Category = "Hover Movement|Input")
	void StrafeRight(bool bPressed);

	/**
	 * Hold-to-boost input (keyboard Left Shift or ESP32 turbo start/end).
	 * Boost activates when held with enough energy; ends on release or empty tank.
	 * @param bPressed - True while boost is requested
	 */
	UFUNCTION(BlueprintCallable, Category = "Hover Movement|Input")
	void SetBoostInput(bool bPressed);

	// ============================================================================
	// STATE QUERY FUNCTIONS
	// ============================================================================

	/**
	 * Get current throttle value (-1 to +1)
	 * @return Current smoothed throttle
	 */
	UFUNCTION(BlueprintPure, Category = "Hover Movement|State")
	float GetCurrentThrottle() const;

	/**
	 * Get current steering value (-1 to +1)
	 * @return Current smoothed steering
	 */
	UFUNCTION(BlueprintPure, Category = "Hover Movement|State")
	float GetCurrentSteering() const;

	/**
	 * Get current turn-bank amount (-1 to +1, positive = bank right / into a right turn)
	 * @return Current smoothed bank amount
	 */
	UFUNCTION(BlueprintPure, Category = "Hover Movement|State")
	float GetCurrentBankAmount() const;

	/**
	 * Get current strafe value (-1 to +1)
	 * @return Current smoothed strafe
	 */
	UFUNCTION(BlueprintPure, Category = "Hover Movement|State")
	float GetCurrentStrafe() const;

	/**
	 * Get the raw throttle input (before smoothing)
	 * @return Raw throttle input
	 */
	UFUNCTION(BlueprintPure, Category = "Hover Movement|State")
	float GetThrottleInput() const;

	/**
	 * Get the raw steering input (before smoothing)
	 * @return Raw steering input
	 */
	UFUNCTION(BlueprintPure, Category = "Hover Movement|State")
	float GetSteeringInput() const;

	/**
	 * Get current forward speed (positive = forward, negative = backward)
	 * @return Speed in cm/s
	 */
	UFUNCTION(BlueprintPure, Category = "Hover Movement|State")
	float GetForwardSpeed() const;

	/**
	 * Get current lateral speed (positive = right, negative = left)
	 * @return Speed in cm/s
	 */
	UFUNCTION(BlueprintPure, Category = "Hover Movement|State")
	float GetLateralSpeed() const;

	/**
	 * Get current speed magnitude (absolute)
	 * @return Speed in cm/s
	 */
	UFUNCTION(BlueprintPure, Category = "Hover Movement|State")
	float GetSpeed() const;

	/**
	 * Check if vehicle is currently grounded (any thruster detecting ground)
	 * @return True if grounded
	 */
	UFUNCTION(BlueprintPure, Category = "Hover Movement|State")
	bool IsGrounded() const;

	/**
	 * Whether boost is currently applying the thrust multiplier
	 */
	UFUNCTION(BlueprintPure, Category = "Hover Movement|Boost")
	bool IsBoostActive() const;

	/**
	 * Remaining boost energy (same units as MaxBoostEnergy)
	 */
	UFUNCTION(BlueprintPure, Category = "Hover Movement|Boost")
	float GetBoostEnergy() const;

	/**
	 * Remaining boost energy as 0-1 fraction of MaxBoostEnergy
	 */
	UFUNCTION(BlueprintPure, Category = "Hover Movement|Boost")
	float GetBoostEnergyNormalized() const;

	// ============================================================================
	// CONTROL FUNCTIONS
	// ============================================================================

	/**
	 * Enable or disable movement processing
	 * @param bEnabled - Whether movement should be processed
	 */
	UFUNCTION(BlueprintCallable, Category = "Hover Movement")
	void SetMovementEnabled(bool bEnabled);

	/**
	 * Check if movement is enabled
	 * @return True if enabled
	 */
	UFUNCTION(BlueprintPure, Category = "Hover Movement")
	bool IsMovementEnabled() const;

	/**
	 * Reset all input to zero immediately (emergency stop)
	 */
	UFUNCTION(BlueprintCallable, Category = "Hover Movement")
	void ResetInput();

	/**
	 * Manually register a hover thruster component
	 * Thrusters are auto-detected at BeginPlay, but this allows manual registration
	 * @param Thruster - The thruster to register
	 */
	UFUNCTION(BlueprintCallable, Category = "Hover Movement")
	void RegisterThruster(UHoverThrusterComponent* Thruster);

	/**
	 * Unregister a hover thruster component
	 * @param Thruster - The thruster to unregister
	 */
	UFUNCTION(BlueprintCallable, Category = "Hover Movement")
	void UnregisterThruster(UHoverThrusterComponent* Thruster);

protected:
	/** Find and register all hover thrusters on the owning actor */
	void AutoRegisterThrusters();

	/** Apply thrust force based on current throttle */
	void ApplyThrust(float DeltaTime);

	/** Apply turning torque based on current steering */
	void ApplyTurning(float DeltaTime);

	/** Bias thruster hover heights so the craft banks into turns */
	void ApplyTurnBank(float DeltaTime);

	/** Clear thruster height offsets applied by turn-bank */
	void ClearTurnBankOffsets();

	/** Apply strafe force based on current strafe input */
	void ApplyStrafeForce(float DeltaTime);

	/** Apply drag forces */
	void ApplyDrag(float DeltaTime);

	/** Update input smoothing */
	void UpdateInputSmoothing(float DeltaTime);

	/** Update boost active state and energy drain/recharge */
	void UpdateBoost(float DeltaTime);

	/** Set boost active state and broadcast if changed */
	void SetBoostActive(bool bNewActive);

	/** Get the primitive component for physics operations */
	UPrimitiveComponent* GetPhysicsComponent() const;

	/** Calculate turn multiplier based on speed */
	float GetSpeedBasedTurnMultiplier() const;

	/** Calculate bank multiplier based on speed */
	float GetSpeedBasedBankMultiplier() const;

	/**
	 * Axis used for yaw/steering torque.
	 * Uses thruster ground normals when available so turning stays heading-stable while banked,
	 * instead of rotating around the tilted actor-up axis (which fights thrusters and unwinds the turn).
	 */
	FVector GetTurnAxis() const;

private:
	/** Whether movement is enabled */
	bool bMovementEnabled = true;

	/** Registered hover thrusters */
	UPROPERTY()
	TArray<UHoverThrusterComponent*> RegisteredThrusters;

	/** Raw input values (before smoothing) */
	float RawThrottleInput = 0.0f;
	float RawSteeringInput = 0.0f;
	float RawStrafeInput = 0.0f;

	/** External analog latch (dual sticks) — applied in Tick before smoothing */
	bool bExternalAnalogActive = false;
	float ExternalThrottleInput = 0.0f;
	float ExternalSteeringInput = 0.0f;
	float ExternalStrafeInput = 0.0f;

	/** Smoothed/current values */
	float CurrentThrottle = 0.0f;
	float CurrentSteering = 0.0f;
	float CurrentStrafe = 0.0f;

	/** Smoothed bank amount (-1 left bank ... +1 right bank) */
	float CurrentBankAmount = 0.0f;

	/** Digital input tracking */
	bool bForwardPressed = false;
	bool bBackwardPressed = false;
	bool bLeftPressed = false;
	bool bRightPressed = false;
	bool bStrafeLeftPressed = false;
	bool bStrafeRightPressed = false;

	/** Boost requested by keyboard or hardware (hold semantics) */
	bool bBoostInputHeld = false;

	/** Whether boost is currently applying thrust multiplier */
	bool bBoostActive = false;

	/** Remaining boost energy */
	float BoostEnergy = 3.0f;

	/** Cached physics component */
	UPROPERTY()
	UPrimitiveComponent* CachedPhysicsComponent = nullptr;
};
