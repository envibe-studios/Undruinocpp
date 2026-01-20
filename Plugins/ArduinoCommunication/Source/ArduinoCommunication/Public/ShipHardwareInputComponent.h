// Arduino Communication Plugin - Ship Hardware Input Component
// ActorComponent that filters UAndySerialSubsystem events by ShipId and exposes friendly Blueprint events

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EspPacketBP.h"
#include "ShipHardwareInputComponent.generated.h"

// Forward declarations
class UAndySerialSubsystem;

// ============================================================================
// Event Delegates - Friendly Blueprint events for ship hardware input
// ============================================================================

/**
 * Event fired when weapon IMU data is received
 * @param Src - Source identifier from the packet
 * @param Orientation - Weapon orientation as quaternion
 * @param bTriggerHeld - True if trigger button is pressed
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FOnWeaponImu,
	uint8, Src,
	FQuat, Orientation,
	bool, bTriggerHeld
);

/**
 * Event fired when wheel turn input is received
 * @param Src - Source identifier from the packet
 * @param Delta - Turn delta (+1 or -1 for direction)
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnWheelTurn,
	uint8, Src,
	int32, Delta
);

/**
 * Event fired when weapon tag is inserted or removed
 * @param Src - Source identifier from the packet
 * @param TagId - RFID/NFC tag unique identifier
 * @param bInserted - True if tag was inserted, false if removed
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FOnWeaponTag,
	uint8, Src,
	int64, TagId,
	bool, bInserted
);

/**
 * Event fired when reload tag is inserted or removed
 * @param Src - Source identifier from the packet
 * @param TagId - RFID/NFC tag unique identifier
 * @param bInserted - True if tag was inserted, false if removed
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FOnReloadTag,
	uint8, Src,
	int64, TagId,
	bool, bInserted
);

/**
 * Event fired when connection status changes for this ship
 * @param bConnected - True if connected, false if disconnected
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnShipConnectionChanged,
	bool, bConnected
);

/**
 * Ship Hardware Input Component
 *
 * Provides a clean interface between hardware input from an ESP32 "Andy" hub
 * and ship gameplay logic. Attach this component to your Ship Pawn/Actor and
 * configure the ShipId to match the port registered in UAndySerialSubsystem.
 *
 * Features:
 * - Filters frames by ShipId (only receives events for this ship)
 * - Parses packet payloads into typed, Blueprint-friendly events
 * - Server-only operation (respects authority for networked games)
 *
 * Exposed Events:
 * - OnWeaponImu: IMU orientation + trigger state from weapon controllers
 * - OnWheelTurn: Rotary encoder input from steering/helm wheels
 * - OnWeaponTag: RFID/NFC weapon tag insertion/removal
 * - OnReloadTag: RFID/NFC reload tag insertion/removal
 * - OnShipConnectionChanged: ESP32 connection status
 *
 * Usage:
 *   1. Add UShipHardwareInputComponent to your Ship Actor
 *   2. Set ShipId to match the port name (e.g., "ShipA")
 *   3. Bind to the events you need in Blueprint
 *   4. Ensure UAndySerialSubsystem has the port registered and started
 */
UCLASS(ClassGroup=(Hardware), meta=(BlueprintSpawnableComponent), BlueprintType, Blueprintable)
class ARDUINOCOMMUNICATION_API UShipHardwareInputComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UShipHardwareInputComponent();

	// === Configuration ===

	/** Ship identifier - must match the ShipId used in UAndySerialSubsystem::AddPort */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship Hardware|Config")
	FName ShipId;

	/** If true, only bind and process events on the server (HasAuthority check) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship Hardware|Config")
	bool bServerOnly = true;

	// === Events ===

	/** Event fired when weapon IMU data is received (orientation + trigger) */
	UPROPERTY(BlueprintAssignable, Category = "Ship Hardware|Events")
	FOnWeaponImu OnWeaponImu;

	/** Event fired when wheel turn input is received */
	UPROPERTY(BlueprintAssignable, Category = "Ship Hardware|Events")
	FOnWheelTurn OnWheelTurn;

	/** Event fired when weapon tag is inserted or removed */
	UPROPERTY(BlueprintAssignable, Category = "Ship Hardware|Events")
	FOnWeaponTag OnWeaponTag;

	/** Event fired when reload tag is inserted or removed */
	UPROPERTY(BlueprintAssignable, Category = "Ship Hardware|Events")
	FOnReloadTag OnReloadTag;

	/** Event fired when connection status changes for this ship's port */
	UPROPERTY(BlueprintAssignable, Category = "Ship Hardware|Events")
	FOnShipConnectionChanged OnShipConnectionChanged;

	// === Status ===

	/**
	 * Check if this ship's serial port is currently connected
	 * @return True if connected
	 */
	UFUNCTION(BlueprintPure, Category = "Ship Hardware|Status")
	bool IsConnected() const;

	/**
	 * Get the subsystem managing serial connections
	 * @return The UAndySerialSubsystem instance, or nullptr if not available
	 */
	UFUNCTION(BlueprintPure, Category = "Ship Hardware|Status")
	UAndySerialSubsystem* GetSerialSubsystem() const;

protected:
	// === UActorComponent Interface ===

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// === Internal Event Handlers ===

	/** Handler for raw frame events from subsystem - filters by ShipId and dispatches */
	UFUNCTION()
	void OnFrameParsedHandler(FName InShipId, uint8 Src, uint8 Type, int32 Seq, const TArray<uint8>& Payload);

	/** Handler for connection changed events from subsystem - filters by ShipId */
	UFUNCTION()
	void OnConnectionChangedHandler(FName InShipId, bool bConnected);

private:
	/** Cached reference to the serial subsystem */
	UPROPERTY()
	TObjectPtr<UAndySerialSubsystem> CachedSubsystem;

	/** Whether we are currently bound to subsystem events */
	bool bIsBound = false;

	/** Bind to the subsystem's delegates */
	void BindToSubsystem();

	/** Unbind from the subsystem's delegates */
	void UnbindFromSubsystem();
};
