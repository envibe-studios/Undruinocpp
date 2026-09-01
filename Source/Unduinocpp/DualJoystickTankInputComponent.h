// DualJoystickTankInputComponent - Tank-style dual-stick input for hover vehicles
// Reads two DirectInput joysticks (e.g. Logitech Attack 3) via Windows joy IDs,
// mixes Forward/Yaw/Strafe, and feeds UHoverMovementComponent.
// Intentionally bypasses Unreal Gamepad/Enhanced Input device merging for identical USB sticks.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DualJoystickTankInputComponent.generated.h"

class UHoverMovementComponent;

/**
 * Polls two Windows joystick device indices (Joy0/Joy1 by default), normalizes axes,
 * mixes tank-style Forward/Yaw + averaged Strafe, and drives UHoverMovementComponent.
 *
 * Keyboard / BPI_ESPComm controls remain usable when sticks are near center
 * (this component only writes movement input while sticks are active, plus one zeroing frame).
 */
UCLASS(ClassGroup = (Vehicle), meta = (BlueprintSpawnableComponent), BlueprintType, Blueprintable)
class UNDUINOCPP_API UDualJoystickTankInputComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDualJoystickTankInputComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ============================================================================
	// ENABLE / DEVICE BINDING
	// ============================================================================

	/** Master enable. When false, no polling and no movement writes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dual Stick|Enable")
	bool bEnabled = true;

	/** If true, push mixed values into UHoverMovementComponent on the owner. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dual Stick|Enable")
	bool bApplyToHoverMovement = true;

	/**
	 * If true and a HoverMovementComponent is found, force bEnableStrafe on at BeginPlay
	 * so lateral stick input can take effect.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dual Stick|Enable")
	bool bAutoEnableStrafe = true;

	/**
	 * When auto-enabling strafe, if MaxStrafeThrust is below this fraction of MaxForwardThrust,
	 * raise it to (MaxForwardThrust * StrafeThrustScale). Avoids invisible strafe when the
	 * craft was tuned with huge forward thrust and default MaxStrafeThrust (15k).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dual Stick|Enable", meta = (ClampMin = "0.05", ClampMax = "1.0", EditCondition = "bAutoEnableStrafe"))
	float StrafeThrustScale = 0.35f;

	/**
	 * If true, both sticks' boost buttons must be held to request boost via SetBoostInput.
	 * Releasing either button cancels boost. Keyboard boost still works when sticks are not holding.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dual Stick|Boost")
	bool bApplyBoostFromJoysticks = true;

	/**
	 * 1-based button index matching Windows / joy.cpl (JOYBUTTON1 = 1).
	 * Attack 3 "button 3" is typically index 3.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dual Stick|Boost", meta = (ClampMin = "1", ClampMax = "32", EditCondition = "bApplyBoostFromJoysticks"))
	int32 BoostButtonIndex = 3;

	/**
	 * Windows joystick ID for the left (port) stick.
	 * Attack 3 devices appear as separate JoyN indices; they do not map cleanly as two Gamepad_* devices.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dual Stick|Devices", meta = (ClampMin = "0", ClampMax = "15"))
	int32 LeftJoystickId = 0;

	/** Windows joystick ID for the right (starboard) stick. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dual Stick|Devices", meta = (ClampMin = "0", ClampMax = "15"))
	int32 RightJoystickId = 1;

	/**
	 * Swap left/right device assignment without re-plugging.
	 * Useful when USB enumeration order does not match chair mount sides.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dual Stick|Devices")
	bool bSwapLeftRightDevices = false;

	// ============================================================================
	// AXIS INVERSION
	// ============================================================================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dual Stick|Inversion")
	bool bInvertLeftX = false;

	/** Attack 3 often reports forward as negative Y; default invert so forward = +1. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dual Stick|Inversion")
	bool bInvertLeftY = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dual Stick|Inversion")
	bool bInvertRightX = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dual Stick|Inversion")
	bool bInvertRightY = true;

	// ============================================================================
	// DEADZONE / RESPONSE
	// ============================================================================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dual Stick|Tuning", meta = (ClampMin = "0.0", ClampMax = "0.9"))
	float LeftXDeadzone = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dual Stick|Tuning", meta = (ClampMin = "0.0", ClampMax = "0.9"))
	float LeftYDeadzone = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dual Stick|Tuning", meta = (ClampMin = "0.0", ClampMax = "0.9"))
	float RightXDeadzone = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dual Stick|Tuning", meta = (ClampMin = "0.0", ClampMax = "0.9"))
	float RightYDeadzone = 0.12f;

	/** Multiplier on mixed forward/reverse (average of Y axes). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dual Stick|Tuning", meta = (ClampMin = "0.0", ClampMax = "4.0"))
	float ForwardSensitivity = 1.0f;

	/** Multiplier on mixed yaw (difference of Y axes). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dual Stick|Tuning", meta = (ClampMin = "0.0", ClampMax = "4.0"))
	float YawSensitivity = 1.0f;

	/** Multiplier on mixed strafe (average of X axes). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dual Stick|Tuning", meta = (ClampMin = "0.0", ClampMax = "4.0"))
	float StrafeSensitivity = 1.0f;

	/**
	 * Response curve exponent applied after deadzone (1 = linear).
	 * Values > 1 soften near center (good for worn sticks).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dual Stick|Tuning", meta = (ClampMin = "0.25", ClampMax = "4.0"))
	float ResponseExponent = 1.2f;

	/** Absolute clamp applied to each normalized axis and to mixed outputs. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dual Stick|Tuning", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float MaxInputClamp = 1.0f;

	/**
	 * Invert mixed yaw sign if physical tank feel is mirrored relative to hovercraft steering
	 * (+steering = turn right in UHoverMovementComponent).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dual Stick|Tuning")
	bool bInvertMixedYaw = false;

	/**
	 * Invert mixed strafe sign if needed for craft local +Y right convention.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dual Stick|Tuning")
	bool bInvertMixedStrafe = false;

	// ============================================================================
	// DEBUG
	// ============================================================================

	/** On-screen raw + processed values (toggleable). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dual Stick|Debug")
	bool bDebugDisplay = false;

	/** Also PrintString/log each debug refresh while display is on. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dual Stick|Debug")
	bool bDebugLog = false;

	/** Seconds between log lines when bDebugLog is true. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dual Stick|Debug", meta = (ClampMin = "0.05", ClampMax = "2.0"))
	float DebugLogInterval = 0.25f;

	/** Optional key to toggle bDebugDisplay at runtime (None = disabled). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dual Stick|Debug")
	FKey DebugToggleKey = EKeys::F8;

	// ============================================================================
	// STATE QUERIES
	// ============================================================================

	UFUNCTION(BlueprintPure, Category = "Dual Stick|State")
	float GetRawLeftX() const { return RawLeftX; }

	UFUNCTION(BlueprintPure, Category = "Dual Stick|State")
	float GetRawLeftY() const { return RawLeftY; }

	UFUNCTION(BlueprintPure, Category = "Dual Stick|State")
	float GetRawRightX() const { return RawRightX; }

	UFUNCTION(BlueprintPure, Category = "Dual Stick|State")
	float GetRawRightY() const { return RawRightY; }

	UFUNCTION(BlueprintPure, Category = "Dual Stick|State")
	float GetLeftX() const { return LeftX; }

	UFUNCTION(BlueprintPure, Category = "Dual Stick|State")
	float GetLeftY() const { return LeftY; }

	UFUNCTION(BlueprintPure, Category = "Dual Stick|State")
	float GetRightX() const { return RightX; }

	UFUNCTION(BlueprintPure, Category = "Dual Stick|State")
	float GetRightY() const { return RightY; }

	UFUNCTION(BlueprintPure, Category = "Dual Stick|State")
	float GetForwardInput() const { return ForwardInput; }

	UFUNCTION(BlueprintPure, Category = "Dual Stick|State")
	float GetYawInput() const { return YawInput; }

	UFUNCTION(BlueprintPure, Category = "Dual Stick|State")
	float GetStrafeInput() const { return StrafeInput; }

	UFUNCTION(BlueprintPure, Category = "Dual Stick|State")
	bool IsLeftJoystickConnected() const { return bLeftConnected; }

	UFUNCTION(BlueprintPure, Category = "Dual Stick|State")
	bool IsRightJoystickConnected() const { return bRightConnected; }

	UFUNCTION(BlueprintPure, Category = "Dual Stick|State")
	FString GetLeftDeviceName() const { return LeftDeviceName; }

	UFUNCTION(BlueprintPure, Category = "Dual Stick|State")
	FString GetRightDeviceName() const { return RightDeviceName; }

	UFUNCTION(BlueprintPure, Category = "Dual Stick|State")
	bool IsLeftBoostButtonDown() const { return bLeftBoostButtonDown; }

	UFUNCTION(BlueprintPure, Category = "Dual Stick|State")
	bool IsRightBoostButtonDown() const { return bRightBoostButtonDown; }

	UFUNCTION(BlueprintPure, Category = "Dual Stick|State")
	bool IsDualBoostRequested() const { return bLeftBoostButtonDown && bRightBoostButtonDown; }

	UFUNCTION(BlueprintCallable, Category = "Dual Stick|Debug")
	void SetDebugDisplay(bool bEnable);

	UFUNCTION(BlueprintCallable, Category = "Dual Stick|Debug")
	void ToggleDebugDisplay();

protected:
	void ResolveMovementComponent();
	void PollDevices();
	void MixControls();
	void ApplyToMovement();
	void ApplyBoost();
	void HandleDebugToggle();
	void DrawDebug() const;

	/** Reads X/Y and whether BoostButtonIndex is down (1-based). */
	static bool ReadJoystickState(int32 JoyId, int32 ButtonIndex1Based, float& OutX, float& OutY, bool& OutButtonDown, FString& OutName, bool& OutConnected);
	static float NormalizeAxis(float RawMinus1To1, bool bInvert, float Deadzone, float Exponent, float ClampMax);

private:
	UPROPERTY()
	TObjectPtr<UHoverMovementComponent> CachedMovement = nullptr;

	float RawLeftX = 0.0f;
	float RawLeftY = 0.0f;
	float RawRightX = 0.0f;
	float RawRightY = 0.0f;

	float LeftX = 0.0f;
	float LeftY = 0.0f;
	float RightX = 0.0f;
	float RightY = 0.0f;

	float ForwardInput = 0.0f;
	float YawInput = 0.0f;
	float StrafeInput = 0.0f;

	bool bLeftConnected = false;
	bool bRightConnected = false;
	bool bLeftBoostButtonDown = false;
	bool bRightBoostButtonDown = false;
	FString LeftDeviceName;
	FString RightDeviceName;

	/** True while sticks are driving movement, or we still owe a zeroing write. */
	bool bDrivingMovement = false;

	/** True while dual-stick boost was forcing SetBoostInput (need one false write on release). */
	bool bDrivingBoost = false;

	float DebugLogAccumulator = 0.0f;
	bool bDebugToggleKeyWasDown = false;
};
