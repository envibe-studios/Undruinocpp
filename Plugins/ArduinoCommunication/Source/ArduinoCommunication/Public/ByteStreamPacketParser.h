// Arduino Communication Plugin - Binary Packet Parser
// Provides Blueprint-safe binary packet parsing layer for serial/TCP data streams

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ByteStreamPacketParser.generated.h"

/**
 * Decoded packet structure for bench/sensor data
 * Framed packet format: [0xAA][VER][SRC][TYPE][SEQ_L][SEQ_H][LEN][PAYLOAD...][CRC][0x55]
 *
 * Header (6 bytes after start): VER(1), SRC(1), TYPE(1), SEQ_L(1), SEQ_H(1), LEN(1)
 * Payload: 0..32 bytes
 * CRC: XOR of bytes 1..(6+LEN), i.e., all header bytes and payload
 * Total frame length: 1(start) + 6(header) + LEN(payload) + 1(CRC) + 1(end) = 9 + LEN bytes
 */
USTRUCT(BlueprintType)
struct ARDUINOCOMMUNICATION_API FBenchPacket
{
	GENERATED_BODY()

	/** Protocol version */
	UPROPERTY(BlueprintReadOnly, Category = "Packet")
	uint8 Ver = 0;

	/** Source identifier */
	UPROPERTY(BlueprintReadOnly, Category = "Packet")
	uint8 Src = 0;

	/** Packet type */
	UPROPERTY(BlueprintReadOnly, Category = "Packet")
	uint8 Type = 0;

	/** Sequence number (uint16 little-endian from SEQ_L | SEQ_H << 8) */
	UPROPERTY(BlueprintReadOnly, Category = "Packet")
	int32 Seq = 0;

	/** Payload length (0..32) */
	UPROPERTY(BlueprintReadOnly, Category = "Packet")
	uint8 Len = 0;

	/** Payload data bytes */
	UPROPERTY(BlueprintReadOnly, Category = "Packet")
	TArray<uint8> Payload;

	FBenchPacket() = default;

	FBenchPacket(uint8 InVer, uint8 InSrc, uint8 InType, int32 InSeq, uint8 InLen, const TArray<uint8>& InPayload)
		: Ver(InVer), Src(InSrc), Type(InType), Seq(InSeq), Len(InLen), Payload(InPayload)
	{
	}
};

/** Delegate fired when a packet is successfully decoded */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBenchPacketDecoded, FBenchPacket, Packet);

/** Delegate fired when bytes are dropped (junk or buffer overflow) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBytesDropped, int32, ByteCount);

/** Delegate fired when a bad end frame is detected */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBadEndFrame);

/** Delegate fired when a CRC mismatch is detected */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCrcMismatch, uint8, Expected, uint8, Actual);

/**
 * Stateful byte stream packet parser
 * Buffers incoming byte chunks and decodes variable-length framed packets
 *
 * Packet Format:
 *   Byte 0: 0xAA (START)
 *   Byte 1: VER (protocol version)
 *   Byte 2: SRC (source identifier)
 *   Byte 3: TYPE (packet type)
 *   Byte 4: SEQ_L (sequence number low byte)
 *   Byte 5: SEQ_H (sequence number high byte)
 *   Byte 6: LEN (payload length, 0..32)
 *   Bytes 7..(7+LEN-1): PAYLOAD (variable length)
 *   Byte 7+LEN: CRC (XOR of bytes 1..(6+LEN))
 *   Byte 8+LEN: 0x55 (END)
 *
 * Total frame size: 9 + LEN bytes
 *
 * Features:
 *   - Robust resync on malformed data by scanning for 0xAA
 *   - CRC validation (XOR of header and payload bytes)
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

	/** Header size in bytes (VER, SRC, TYPE, SEQ_L, SEQ_H, LEN) */
	static constexpr int32 HeaderSize = 6;

	/** Minimum frame size (START + HEADER + CRC + END, with LEN=0) */
	static constexpr int32 MinFrameSize = 9;

	/** Minimum bytes needed to read header (START + HEADER) */
	static constexpr int32 MinBytesToReadHeader = 7;

	/** Maximum payload length */
	static constexpr int32 MaxPayloadLen = 32;

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

	/** Event fired when a CRC mismatch is detected */
	UPROPERTY(BlueprintAssignable, Category = "Parser|Events")
	FOnCrcMismatch OnCrcMismatch;

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
	 * @param OutCrcMismatches - Number of CRC validation failures
	 * @return Number of packets successfully decoded
	 */
	UFUNCTION(BlueprintCallable, Category = "Parser")
	int32 ParsePackets(TArray<FBenchPacket>& OutPackets, int32& OutBytesDropped, int32& OutBadEndFrames, int32& OutCrcMismatches);

	/**
	 * Append bytes and immediately parse (convenience method)
	 * @param InBytes - Raw byte array to process
	 * @param OutPackets - Array to receive decoded packets
	 * @param OutBytesDropped - Number of bytes discarded
	 * @param OutBadEndFrames - Number of bad end frame markers detected
	 * @param OutCrcMismatches - Number of CRC validation failures
	 * @return Number of packets successfully decoded
	 */
	UFUNCTION(BlueprintCallable, Category = "Parser")
	int32 IngestAndParse(const TArray<uint8>& InBytes, TArray<FBenchPacket>& OutPackets, int32& OutBytesDropped, int32& OutBadEndFrames, int32& OutCrcMismatches);

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

	/** Total CRC mismatches since creation/reset */
	UPROPERTY(BlueprintReadOnly, Category = "Parser|Stats")
	int64 TotalCrcMismatches = 0;

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
	 * Assumes caller has validated there are enough bytes available
	 * @param Offset - Buffer offset to read from
	 * @param PayloadLen - Payload length from header
	 * @return Decoded packet
	 */
	FBenchPacket DecodePacketAt(int32 Offset, int32 PayloadLen);

	/**
	 * Compute CRC (XOR of bytes from Offset+1 to Offset+6+PayloadLen)
	 * @param Offset - Buffer offset of START byte
	 * @param PayloadLen - Payload length
	 * @return Computed CRC byte
	 */
	uint8 ComputeCrc(int32 Offset, int32 PayloadLen) const;

	/**
	 * Trim buffer if it exceeds maximum size
	 * @return Number of bytes trimmed
	 */
	int32 EnforceBufferLimits();
};
