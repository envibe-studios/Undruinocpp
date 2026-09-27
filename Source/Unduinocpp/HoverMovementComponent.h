// Hover Movement Component - Provides movement controls for hover vehicles
// Handles forward/backward thrust, turning, and coordinates with hover thrusters

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

/**
 * Hover Movement Component
 *
 * Provides movement controls for hover vehicles with physics-based thrust and turning.
 * Designed to work with UHoverThrusterComponent for complete hover vehicle control.
 *
 * Features:
 * - Forward/backward thrust with gradual input support (pedals, triggers)
 * - Left/right turning with gradual input support (steering wheels, joysticks)
 * - Configurable acceleration and deceleration curves
 * - Input smoothing for natural feel
 * - Full Blueprint exposure for input binding
 *
 * Usage:
 *   1. Add UHoverMovementComponent to your hover vehicle pawn
 *   2. Configure thrust and turn forces
 *   3. Bind SetThrottleInput() and SetSteeringInput() to input actions
 *   4. For digital input, use MoveForward/MoveBackward/TurnLeft/TurnRight
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

	/** Apply strafe force based on current strafe input */
	void ApplyStrafeForce(float DeltaTime);

	/** Apply drag forces */
	void ApplyDrag(float DeltaTime);

	/** Update input smoothing */
	void UpdateInputSmoothing(float DeltaTime);

	/** Get the primitive component for physics operations */
	UPrimitiveComponent* GetPhysicsComponent() const;

	/** Calculate turn multiplier based on speed */
	float GetSpeedBasedTurnMultiplier() const;

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

	/** Smoothed/current values */
	float CurrentThrottle = 0.0f;
	float CurrentSteering = 0.0f;
	float CurrentStrafe = 0.0f;

	/** Digital input tracking */
	bool bForwardPressed = false;
	bool bBackwardPressed = false;
	bool bLeftPressed = false;
	bool bRightPressed = false;
	bool bStrafeLeftPressed = false;
	bool bStrafeRightPressed = false;

	/** Cached physics component */
	UPROPERTY()
	UPrimitiveComponent* CachedPhysicsComponent = nullptr;
};
