// Arduino Communication Plugin - Binary Packet Parser
// Provides Blueprint-safe binary packet parsing layer for serial/TCP data streams

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ByteStreamPacketParser.generated.h"

/**
 * Decoded packet structure for bench/sensor data
 * Fixed 6-byte packet format: [0xAA][cycleLo][cycleHi][line][data][0x55]
 */
USTRUCT(BlueprintType)
struct ARDUINOCOMMUNICATION_API FBenchPacket
{
	GENERATED_BODY()

	/** Cycle counter (uint16 little-endian from cycleLo | cycleHi << 8) */
	UPROPERTY(BlueprintReadOnly, Category = "Packet")
	int32 Cycle = 0;

	/** Line identifier */
	UPROPERTY(BlueprintReadOnly, Category = "Packet")
	uint8 Line = 0;

	/** Data payload byte */
	UPROPERTY(BlueprintReadOnly, Category = "Packet")
	uint8 Data = 0;

	FBenchPacket() = default;

	FBenchPacket(int32 InCycle, uint8 InLine, uint8 InData)
		: Cycle(InCycle), Line(InLine), Data(InData)
	{
	}
};

/** Delegate fired when a packet is successfully decoded */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBenchPacketDecoded, FBenchPacket, Packet);

/** Delegate fired when bytes are dropped (junk or buffer overflow) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBytesDropped, int32, ByteCount);

/** Delegate fired when a bad end frame is detected */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBadEndFrame);

/**
 * Stateful byte stream packet parser
 * Buffers incoming byte chunks and decodes fixed-format 6-byte packets
 *
 * Packet Format:
 *   Byte 0: 0xAA (START)
 *   Byte 1: cycleLo (uint8)
 *   Byte 2: cycleHi (uint8)
 *   Byte 3: line (uint8)
 *   Byte 4: data (uint8)
 *   Byte 5: 0x55 (END)
 *
 * Features:
 *   - Robust resync on malformed data
 *   - Bounded memory usage with configurable limits
 *   - Blueprint-safe events for decoded packets
 */
UCLASS(BlueprintType, Blueprintable)
class ARDUINOCOMMUNICATION_API UByteStreamPacketParser : public UObject
{
	GENERATED_BODY()

public:
	UByteStreamPacketParser();

	// === Protocol Constants ===

	/** Start byte marker (0xAA) */
	static constexpr uint8 StartByte = 0xAA;

	/** End byte marker (0x55) */
	static constexpr uint8 EndByte = 0x55;

	/** Fixed packet size in bytes */
	static constexpr int32 PacketSize = 6;

	// === Configuration ===

	/** Maximum buffer size before aggressive trimming (bytes) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parser|Config")
	int32 MaxBufferBytes = 4096;

	/** Number of bytes to keep when trimming buffer */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parser|Config")
	int32 TrimToBytes = 64;

	/** Maximum packets to parse per call (prevents infinite loops/stalls) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parser|Config")
	int32 MaxPacketsPerCall = 200;

	/** Whether to broadcast OnPacketDecoded during parsing */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parser|Config")
	bool bBroadcastPackets = true;

	/** Enable debug mode for sample packet logging */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parser|Debug")
	bool bDebugMode = false;

	/** Log one sample packet every N packets when debug mode is enabled */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parser|Debug", meta = (EditCondition = "bDebugMode", ClampMin = "1"))
	int32 DebugSampleInterval = 1000;

	// === Events ===

	/** Event fired when a packet is successfully decoded */
	UPROPERTY(BlueprintAssignable, Category = "Parser|Events")
	FOnBenchPacketDecoded OnPacketDecoded;

	/** Event fired when bytes are dropped */
	UPROPERTY(BlueprintAssignable, Category = "Parser|Events")
	FOnBytesDropped OnBytesDropped;

	/** Event fired when a bad end frame is detected */
	UPROPERTY(BlueprintAssignable, Category = "Parser|Events")
	FOnBadEndFrame OnBadEndFrame;

	// === Core API ===

	/**
	 * Append incoming bytes to the internal buffer
	 * Call this when raw bytes arrive from serial/TCP
	 * @param InBytes - Raw byte array to append
	 */
	UFUNCTION(BlueprintCallable, Category = "Parser")
	void AppendBytes(const TArray<uint8>& InBytes);

	/**
	 * Parse buffered data and extract valid packets
	 * @param OutPackets - Array to receive decoded packets
	 * @param OutBytesDropped - Number of bytes discarded (junk/resync)
	 * @param OutBadEndFrames - Number of bad end frame markers detected
	 * @return Number of packets successfully decoded
	 */
	UFUNCTION(BlueprintCallable, Category = "Parser")
	int32 ParsePackets(TArray<FBenchPacket>& OutPackets, int32& OutBytesDropped, int32& OutBadEndFrames);

	/**
	 * Append bytes and immediately parse (convenience method)
	 * @param InBytes - Raw byte array to process
	 * @param OutPackets - Array to receive decoded packets
	 * @param OutBytesDropped - Number of bytes discarded
	 * @param OutBadEndFrames - Number of bad end frame markers detected
	 * @return Number of packets successfully decoded
	 */
	UFUNCTION(BlueprintCallable, Category = "Parser")
	int32 IngestAndParse(const TArray<uint8>& InBytes, TArray<FBenchPacket>& OutPackets, int32& OutBytesDropped, int32& OutBadEndFrames);

	/**
	 * Clear the internal buffer and reset state
	 */
	UFUNCTION(BlueprintCallable, Category = "Parser")
	void ResetBuffer();

	/**
	 * Get current number of buffered bytes
	 * @return Number of bytes currently in buffer
	 */
	UFUNCTION(BlueprintPure, Category = "Parser")
	int32 GetBufferedByteCount() const;

	// === Statistics ===

	/** Total bytes received since creation/reset */
	UPROPERTY(BlueprintReadOnly, Category = "Parser|Stats")
	int64 TotalBytesIn = 0;

	/** Total packets decoded since creation/reset */
	UPROPERTY(BlueprintReadOnly, Category = "Parser|Stats")
	int64 TotalPacketsDecoded = 0;

	/** Total bytes dropped since creation/reset */
	UPROPERTY(BlueprintReadOnly, Category = "Parser|Stats")
	int64 TotalBytesDropped = 0;

	/** Total bad end frames since creation/reset */
	UPROPERTY(BlueprintReadOnly, Category = "Parser|Stats")
	int64 TotalBadEndFrames = 0;

	/** Current buffer size in bytes */
	UFUNCTION(BlueprintPure, Category = "Parser|Stats")
	int32 GetBufferSize() const { return Buffer.Num(); }

	/**
	 * Reset all statistics counters
	 */
	UFUNCTION(BlueprintCallable, Category = "Parser|Stats")
	void ResetStatistics();

protected:
	/** Internal byte buffer */
	TArray<uint8> Buffer;

	/**
	 * Find the first occurrence of StartByte in the buffer
	 * @param StartIndex - Index to start searching from
	 * @return Index of StartByte, or INDEX_NONE if not found
	 */
	int32 FindStartByte(int32 StartIndex = 0) const;

	/**
	 * Decode a single packet from buffer at given offset
	 * Assumes caller has validated there are at least PacketSize bytes available
	 * @param Offset - Buffer offset to read from
	 * @return Decoded packet
	 */
	FBenchPacket DecodePacketAt(int32 Offset);

	/**
	 * Trim buffer if it exceeds maximum size
	 * @return Number of bytes trimmed
	 */
	int32 EnforceBufferLimits();
};
