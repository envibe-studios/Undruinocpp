// Arduino Communication Plugin - ESP Packet Blueprint Library
// Provides Blueprint-friendly payload parsing for decoded ESP packets

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "EspPacketBP.generated.h"

/**
 * ESP Message Type enum for packet type identification
 * Use these constants with Switch on EEspMsgType in Blueprints
 */
UENUM(BlueprintType)
enum class EEspMsgType : uint8
{
	None = 0 UMETA(DisplayName = "None"),
	WheelTurn = 1 UMETA(DisplayName = "Wheel Turn"),
	RepairProgress = 2 UMETA(DisplayName = "Repair Progress"),
	JackState = 3 UMETA(DisplayName = "Jack State"),
	WeaponTag = 4 UMETA(DisplayName = "Weapon Tag"),
	ReloadTag = 5 UMETA(DisplayName = "Reload Tag"),
	WeaponImu = 6 UMETA(DisplayName = "Weapon IMU"),
	// Add more types as needed
	Max = 255 UMETA(Hidden)
};

// ============================================================================
// Payload Structs - BlueprintType for clean output pins
// ============================================================================

/**
 * Wheel Turn payload data
 * Len = 2: [WheelIndex (1), Direction (1)]
 */
USTRUCT(BlueprintType)
struct ARDUINOCOMMUNICATION_API FWheelTurnData
{
	GENERATED_BODY()

	/** Which wheel (0-based index) */
	UPROPERTY(BlueprintReadOnly, Category = "ESP|Payload")
	uint8 WheelIndex = 0;

	/** Direction: true = right/clockwise, false = left/counter-clockwise */
	UPROPERTY(BlueprintReadOnly, Category = "ESP|Payload")
	bool bRight = false;

	FWheelTurnData() = default;
	FWheelTurnData(uint8 InIndex, bool InRight) : WheelIndex(InIndex), bRight(InRight) {}
};

/**
 * Repair Progress payload data
 * Len = 2: [Amount_L (1), Amount_H (1)] (uint16 LE)
 */
USTRUCT(BlueprintType)
struct ARDUINOCOMMUNICATION_API FRepairProgressData
{
	GENERATED_BODY()

	/** Repair amount (0-65535) */
	UPROPERTY(BlueprintReadOnly, Category = "ESP|Payload")
	int32 Amount = 0;

	FRepairProgressData() = default;
	FRepairProgressData(int32 InAmount) : Amount(InAmount) {}
};

/**
 * Jack State payload data
 * Len = 1: [State (1)]
 */
USTRUCT(BlueprintType)
struct ARDUINOCOMMUNICATION_API FJackStateData
{
	GENERATED_BODY()

	/** Jack state value */
	UPROPERTY(BlueprintReadOnly, Category = "ESP|Payload")
	uint8 State = 0;

	FJackStateData() = default;
	FJackStateData(uint8 InState) : State(InState) {}
};

/**
 * Weapon Tag payload data
 * Len = 6: [Side (1), UID (4, LE), Button (1)]
 */
USTRUCT(BlueprintType)
struct ARDUINOCOMMUNICATION_API FWeaponTagData
{
	GENERATED_BODY()

	/** Which side (0 = left, 1 = right, etc.) */
	UPROPERTY(BlueprintReadOnly, Category = "ESP|Payload")
	uint8 Side = 0;

	/** RFID/NFC tag UID (32-bit) - stored as int64 for Blueprint compatibility */
	UPROPERTY(BlueprintReadOnly, Category = "ESP|Payload")
	int64 UID = 0;

	/** Button state or ID */
	UPROPERTY(BlueprintReadOnly, Category = "ESP|Payload")
	uint8 Button = 0;

	FWeaponTagData() = default;
	FWeaponTagData(uint8 InSide, int64 InUID, uint8 InButton)
		: Side(InSide), UID(InUID), Button(InButton) {}
};

/**
 * Reload Tag payload data
 * Len = 4: [UID (4, LE)]
 */
USTRUCT(BlueprintType)
struct ARDUINOCOMMUNICATION_API FReloadTagData
{
	GENERATED_BODY()

	/** RFID/NFC tag UID (32-bit) - stored as int64 for Blueprint compatibility */
	UPROPERTY(BlueprintReadOnly, Category = "ESP|Payload")
	int64 UID = 0;

	FReloadTagData() = default;
	FReloadTagData(int64 InUID) : UID(InUID) {}
};

/**
 * Weapon IMU payload data
 * Len = 6: [Side (1), Pitch (2, LE signed), Yaw (2, LE signed), Buttons (1)]
 */
USTRUCT(BlueprintType)
struct ARDUINOCOMMUNICATION_API FWeaponImuData
{
	GENERATED_BODY()

	/** Which side (0 = left, 1 = right, etc.) */
	UPROPERTY(BlueprintReadOnly, Category = "ESP|Payload")
	uint8 Side = 0;

	/** Pitch angle (signed, typically -180 to 180 scaled) */
	UPROPERTY(BlueprintReadOnly, Category = "ESP|Payload")
	int32 Pitch = 0;

	/** Yaw angle (signed, typically -180 to 180 scaled) */
	UPROPERTY(BlueprintReadOnly, Category = "ESP|Payload")
	int32 Yaw = 0;

	/** Button bitmask */
	UPROPERTY(BlueprintReadOnly, Category = "ESP|Payload")
	uint8 Buttons = 0;

	FWeaponImuData() = default;
	FWeaponImuData(uint8 InSide, int32 InPitch, int32 InYaw, uint8 InButtons)
		: Side(InSide), Pitch(InPitch), Yaw(InYaw), Buttons(InButtons) {}
};

// ============================================================================
// Blueprint Function Library
// ============================================================================

/**
 * Blueprint Function Library for ESP Packet Payload Parsing
 *
 * Provides:
 * - Low-level little-endian byte unpacking helpers
 * - Per-type payload parsers returning typed structs
 * - Type enum for Switch node usage
 *
 * Example Blueprint Flow:
 *   OnPacketDecoded -> Switch on EEspMsgType (Packet.Type)
 *     -> ParseWeaponImuPayload(Packet.Payload) -> Use FWeaponImuData
 */
UCLASS()
class ARDUINOCOMMUNICATION_API UEspPacketBP : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// ========================================================================
	// Low-Level Unpacking Helpers
	// ========================================================================

	/**
	 * Read a uint16 from a byte array at the given offset (little-endian)
	 * @param Bytes - Source byte array
	 * @param Offset - Byte offset to read from
	 * @param bOk - Output: true if read succeeded (enough bytes available)
	 * @return The decoded uint16 value (0 if failed)
	 */
	UFUNCTION(BlueprintPure, Category = "ESP|ByteUtils",
		meta = (DisplayName = "Read UInt16 LE", Keywords = "unpack bytes little endian"))
	static int32 ReadUInt16LE(const TArray<uint8>& Bytes, int32 Offset, bool& bOk);

	/**
	 * Read a signed int16 from a byte array at the given offset (little-endian)
	 * @param Bytes - Source byte array
	 * @param Offset - Byte offset to read from
	 * @param bOk - Output: true if read succeeded (enough bytes available)
	 * @return The decoded int16 value (0 if failed)
	 */
	UFUNCTION(BlueprintPure, Category = "ESP|ByteUtils",
		meta = (DisplayName = "Read Int16 LE", Keywords = "unpack bytes little endian signed"))
	static int32 ReadInt16LE(const TArray<uint8>& Bytes, int32 Offset, bool& bOk);

	/**
	 * Read a uint32 from a byte array at the given offset (little-endian)
	 * @param Bytes - Source byte array
	 * @param Offset - Byte offset to read from
	 * @param bOk - Output: true if read succeeded (enough bytes available)
	 * @return The decoded uint32 value as int64 for Blueprint compatibility (0 if failed)
	 */
	UFUNCTION(BlueprintPure, Category = "ESP|ByteUtils",
		meta = (DisplayName = "Read UInt32 LE", Keywords = "unpack bytes little endian"))
	static int64 ReadUInt32LE(const TArray<uint8>& Bytes, int32 Offset, bool& bOk);

	/**
	 * Convert a byte array to a hex string for debugging
	 * @param Bytes - Source byte array
	 * @return Hex string representation (e.g., "AA 55 01 02")
	 */
	UFUNCTION(BlueprintPure, Category = "ESP|ByteUtils",
		meta = (DisplayName = "Bytes To Hex String", Keywords = "debug hex dump"))
	static FString BytesToHexString(const TArray<uint8>& Bytes);

	/**
	 * Convert a packet type byte to the EEspMsgType enum
	 * @param TypeByte - Raw type byte from packet
	 * @return Corresponding enum value (None if unknown)
	 */
	UFUNCTION(BlueprintPure, Category = "ESP|ByteUtils",
		meta = (DisplayName = "Byte To Msg Type", Keywords = "convert type enum"))
	static EEspMsgType ByteToMsgType(uint8 TypeByte);

	// ========================================================================
	// Per-Type Payload Parsers
	// ========================================================================

	/**
	 * Parse Wheel Turn payload (Type = 1)
	 * Expected Len = 2: [WheelIndex, Direction]
	 * @param Payload - Raw payload bytes from packet
	 * @param OutData - Parsed wheel turn data
	 * @return true if payload was valid and parsed successfully
	 */
	UFUNCTION(BlueprintPure, Category = "ESP|Parsers",
		meta = (DisplayName = "Parse Wheel Turn Payload", Keywords = "wheel rotation"))
	static bool ParseWheelTurnPayload(const TArray<uint8>& Payload, FWheelTurnData& OutData);

	/**
	 * Parse Repair Progress payload (Type = 2)
	 * Expected Len = 2: [Amount_L, Amount_H] (uint16 LE)
	 * @param Payload - Raw payload bytes from packet
	 * @param OutData - Parsed repair progress data
	 * @return true if payload was valid and parsed successfully
	 */
	UFUNCTION(BlueprintPure, Category = "ESP|Parsers",
		meta = (DisplayName = "Parse Repair Progress Payload", Keywords = "repair amount"))
	static bool ParseRepairProgressPayload(const TArray<uint8>& Payload, FRepairProgressData& OutData);

	/**
	 * Parse Jack State payload (Type = 3)
	 * Expected Len = 1: [State]
	 * @param Payload - Raw payload bytes from packet
	 * @param OutData - Parsed jack state data
	 * @return true if payload was valid and parsed successfully
	 */
	UFUNCTION(BlueprintPure, Category = "ESP|Parsers",
		meta = (DisplayName = "Parse Jack State Payload", Keywords = "jack plug"))
	static bool ParseJackStatePayload(const TArray<uint8>& Payload, FJackStateData& OutData);

	/**
	 * Parse Weapon Tag payload (Type = 4)
	 * Expected Len = 6: [Side, UID (4 bytes LE), Button]
	 * @param Payload - Raw payload bytes from packet
	 * @param OutData - Parsed weapon tag data
	 * @return true if payload was valid and parsed successfully
	 */
	UFUNCTION(BlueprintPure, Category = "ESP|Parsers",
		meta = (DisplayName = "Parse Weapon Tag Payload", Keywords = "rfid nfc weapon"))
	static bool ParseWeaponTagPayload(const TArray<uint8>& Payload, FWeaponTagData& OutData);

	/**
	 * Parse Reload Tag payload (Type = 5)
	 * Expected Len = 4: [UID (4 bytes LE)]
	 * @param Payload - Raw payload bytes from packet
	 * @param OutData - Parsed reload tag data
	 * @return true if payload was valid and parsed successfully
	 */
	UFUNCTION(BlueprintPure, Category = "ESP|Parsers",
		meta = (DisplayName = "Parse Reload Tag Payload", Keywords = "rfid nfc reload"))
	static bool ParseReloadTagPayload(const TArray<uint8>& Payload, FReloadTagData& OutData);

	/**
	 * Parse Weapon IMU payload (Type = 6)
	 * Expected Len = 6: [Side, Pitch_L, Pitch_H, Yaw_L, Yaw_H, Buttons]
	 * @param Payload - Raw payload bytes from packet
	 * @param OutData - Parsed weapon IMU data
	 * @return true if payload was valid and parsed successfully
	 */
	UFUNCTION(BlueprintPure, Category = "ESP|Parsers",
		meta = (DisplayName = "Parse Weapon IMU Payload", Keywords = "imu gyro accelerometer"))
	static bool ParseWeaponImuPayload(const TArray<uint8>& Payload, FWeaponImuData& OutData);
};
