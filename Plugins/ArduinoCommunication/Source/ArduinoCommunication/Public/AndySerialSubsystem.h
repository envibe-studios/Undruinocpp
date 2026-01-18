// Arduino Communication Plugin - Andy Serial Subsystem
// GameInstanceSubsystem for managing multiple serial ports across level loads

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ArduinoSerialPort.h"
#include "ByteStreamPacketParser.h"
#include "AndySerialSubsystem.generated.h"

// Forward declarations
class UArduinoSerialPort;
class UByteStreamPacketParser;
class UAndySerialSubsystem;

/**
 * Delegate fired when a frame is parsed from any connected port
 * @param ShipId - Identifier for the ship/port that received this frame
 * @param Src - Source identifier from the packet
 * @param Type - Packet type
 * @param Seq - Sequence number
 * @param Payload - Raw payload bytes
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(
	FOnAndyFrameParsed,
	FName, ShipId,
	uint8, Src,
	uint8, Type,
	int32, Seq,
	const TArray<uint8>&, Payload
);

/**
 * Delegate fired when connection status changes for any port
 * @param ShipId - Identifier for the ship/port
 * @param bConnected - True if connected, false if disconnected
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnAndyConnectionChanged,
	FName, ShipId,
	bool, bConnected
);

/**
 * Helper object that binds to a single serial port's events
 * and forwards them to the subsystem with the correct ShipId
 */
UCLASS()
class UAndyPortEventHandler : public UObject
{
	GENERATED_BODY()

public:
	/** Initialize with owner subsystem and ship identifier */
	void Setup(UAndySerialSubsystem* InOwner, FName InShipId);

	/** Bind to the serial port's events */
	void BindToPort(UArduinoSerialPort* Port);

	/** Unbind from the serial port's events */
	void UnbindFromPort(UArduinoSerialPort* Port);

	/** Get the ShipId this handler is associated with */
	FName GetShipId() const { return ShipId; }

protected:
	UFUNCTION()
	void OnBytesReceived(const TArray<uint8>& Bytes);

	UFUNCTION()
	void OnConnectionChanged(bool bConnected);

private:
	UPROPERTY()
	TObjectPtr<UAndySerialSubsystem> OwnerSubsystem;

	FName ShipId;
};

/**
 * Internal struct to hold per-port connection state
 * Contains the serial port and its associated parser
 */
USTRUCT()
struct FAndyPortConnection
{
	GENERATED_BODY()

	/** The serial port for this connection */
	UPROPERTY()
	TObjectPtr<UArduinoSerialPort> SerialPort;

	/** The packet parser for this connection */
	UPROPERTY()
	TObjectPtr<UByteStreamPacketParser> Parser;

	/** Event handler for routing port events to subsystem */
	UPROPERTY()
	TObjectPtr<UAndyPortEventHandler> EventHandler;

	/** COM port name (e.g., "COM3" or "/dev/ttyUSB0") */
	UPROPERTY()
	FString PortName;

	/** Baud rate for this connection */
	UPROPERTY()
	int32 BaudRate = 115200;

	/** Whether this port should auto-start when StartAll is called */
	UPROPERTY()
	bool bAutoStart = true;

	FAndyPortConnection()
		: SerialPort(nullptr)
		, Parser(nullptr)
		, EventHandler(nullptr)
		, BaudRate(115200)
		, bAutoStart(true)
	{
	}
};

/**
 * Game Instance Subsystem for managing multiple Arduino/ESP serial connections
 *
 * This subsystem persists across level loads and provides a centralized way to:
 * - Manage multiple serial port connections (one per ship/Andy)
 * - Parse incoming byte frames using the existing protocol
 * - Route parsed frames to Blueprints via delegates
 *
 * Usage in Blueprints:
 *   1. Get Game Instance Subsystem -> AndySerialSubsystem
 *   2. Call AddPort("ShipA", "COM3", 115200)
 *   3. Call AddPort("ShipB", "COM4", 115200)
 *   4. Bind to OnFrameParsed and OnConnectionChanged
 *   5. Call StartAll() to open all ports
 */
UCLASS(BlueprintType, Blueprintable)
class ARDUINOCOMMUNICATION_API UAndySerialSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UAndySerialSubsystem();

	// === USubsystem Interface ===

	/** Called when the subsystem is initialized (game instance created) */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** Called when the subsystem is deinitialized (game instance destroyed) */
	virtual void Deinitialize() override;

	// === Port Management ===

	/**
	 * Add a new serial port for a ship
	 * @param ShipId - Unique identifier for this ship (e.g., "ShipA", "ShipB")
	 * @param PortName - COM port name (e.g., "COM3" or "/dev/ttyUSB0")
	 * @param BaudRate - Baud rate (default 115200)
	 * @return True if port was added successfully, false if ShipId already exists
	 */
	UFUNCTION(BlueprintCallable, Category = "Andy|Serial")
	bool AddPort(FName ShipId, const FString& PortName, int32 BaudRate = 115200);

	/**
	 * Remove a serial port for a ship
	 * @param ShipId - Identifier of the ship to remove
	 * @return True if port was removed, false if ShipId not found
	 */
	UFUNCTION(BlueprintCallable, Category = "Andy|Serial")
	bool RemovePort(FName ShipId);

	/**
	 * Start all registered serial ports
	 * Opens connections for all ports added via AddPort
	 */
	UFUNCTION(BlueprintCallable, Category = "Andy|Serial")
	void StartAll();

	/**
	 * Stop all serial ports
	 * Closes all open connections
	 */
	UFUNCTION(BlueprintCallable, Category = "Andy|Serial")
	void StopAll();

	/**
	 * Check if a specific ship's port is connected
	 * @param ShipId - Identifier of the ship to check
	 * @return True if the port is open and connected
	 */
	UFUNCTION(BlueprintPure, Category = "Andy|Serial")
	bool IsConnected(FName ShipId) const;

	/**
	 * Get list of all registered ship IDs
	 * @return Array of ShipId names
	 */
	UFUNCTION(BlueprintPure, Category = "Andy|Serial")
	TArray<FName> GetAllShipIds() const;

	/**
	 * Send raw bytes to a specific ship
	 * @param ShipId - Target ship identifier
	 * @param Data - Bytes to send
	 * @return True if data was sent successfully
	 */
	UFUNCTION(BlueprintCallable, Category = "Andy|Serial")
	bool SendBytes(FName ShipId, const TArray<uint8>& Data);

	/**
	 * Send a text line to a specific ship (appends newline)
	 * @param ShipId - Target ship identifier
	 * @param Line - Text to send
	 * @return True if line was sent successfully
	 */
	UFUNCTION(BlueprintCallable, Category = "Andy|Serial")
	bool SendLine(FName ShipId, const FString& Line);

	// === Events ===

	/** Event fired when a frame is successfully parsed from any port */
	UPROPERTY(BlueprintAssignable, Category = "Andy|Serial|Events")
	FOnAndyFrameParsed OnFrameParsed;

	/** Event fired when connection status changes for any port */
	UPROPERTY(BlueprintAssignable, Category = "Andy|Serial|Events")
	FOnAndyConnectionChanged OnConnectionChanged;

	// === Internal Event Handlers (called by UAndyPortEventHandler) ===

	/**
	 * Internal handler for raw bytes received from a port
	 * Routes bytes to the appropriate parser
	 */
	void HandleBytesReceived(FName ShipId, const TArray<uint8>& Bytes);

	/**
	 * Internal handler for connection status changes
	 * Broadcasts to OnConnectionChanged delegate
	 */
	void HandleConnectionChanged(FName ShipId, bool bConnected);

protected:
	/** Map of ShipId to connection objects */
	UPROPERTY()
	TMap<FName, FAndyPortConnection> Connections;

	/**
	 * Internal handler for parsed packets
	 * Broadcasts to OnFrameParsed delegate
	 */
	void HandlePacketDecoded(FName ShipId, const FBenchPacket& Packet);

private:
	/** Creates and configures a parser instance for a connection */
	UByteStreamPacketParser* CreateParserForConnection(FName ShipId);
};
