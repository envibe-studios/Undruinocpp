// Arduino Communication Plugin - Packet Parser Actor Component
// Provides easy Blueprint integration for binary packet parsing

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ByteStreamPacketParser.h"
#include "PacketParserComponent.generated.h"

/**
 * Actor Component wrapper for UByteStreamPacketParser
 * Simplifies Blueprint integration by providing a component-based interface
 *
 * Usage:
 *   1. Add this component to any actor
 *   2. Connect serial/TCP OnByteReceived to IngestBytes
 *   3. Bind to OnPacketDecoded for parsed packets
 *
 * Example Blueprint wiring:
 *   ArduinoCommunicationComponent.OnByteReceived -> PacketParserComponent.IngestBytes
 *   PacketParserComponent.OnPacketDecoded -> Your handler function
 */
UCLASS(ClassGroup=(Arduino), meta=(BlueprintSpawnableComponent))
class ARDUINOCOMMUNICATION_API UPacketParserComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPacketParserComponent();

	// UActorComponent interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// === Configuration ===

	/** Maximum buffer size before aggressive trimming (bytes) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parser|Config")
	int32 MaxBufferBytes = 4096;

	/** Number of bytes to keep when trimming buffer */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parser|Config")
	int32 TrimToBytes = 64;

	/** Maximum packets to parse per call (prevents infinite loops) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parser|Config")
	int32 MaxPacketsPerCall = 512;

	// === Events ===

	/** Event fired when a packet is successfully decoded */
	UPROPERTY(BlueprintAssignable, Category = "Parser|Events")
	FOnBenchPacketDecoded OnPacketDecoded;

	/** Event fired when bytes are dropped (junk or buffer overflow) */
	UPROPERTY(BlueprintAssignable, Category = "Parser|Events")
	FOnBytesDropped OnBytesDropped;

	/** Event fired when a bad end frame is detected */
	UPROPERTY(BlueprintAssignable, Category = "Parser|Events")
	FOnBadEndFrame OnBadEndFrame;

	// === Core API ===

	/**
	 * Ingest raw bytes from serial/TCP and parse packets
	 * Call this from your OnByteReceived event
	 * @param InBytes - Raw byte array from serial/TCP
	 */
	UFUNCTION(BlueprintCallable, Category = "Parser")
	void IngestBytes(const TArray<uint8>& InBytes);

	/**
	 * Parse any remaining buffered data without adding new bytes
	 * @param OutPackets - Array to receive decoded packets
	 * @return Number of packets decoded
	 */
	UFUNCTION(BlueprintCallable, Category = "Parser")
	int32 FlushAndParse(TArray<FBenchPacket>& OutPackets);

	/**
	 * Clear the internal buffer and reset parser state
	 */
	UFUNCTION(BlueprintCallable, Category = "Parser")
	void ResetParser();

	/**
	 * Get current number of buffered bytes
	 * @return Number of bytes currently in buffer
	 */
	UFUNCTION(BlueprintPure, Category = "Parser")
	int32 GetBufferedByteCount() const;

	/**
	 * Get direct access to the underlying parser object
	 * @return Parser object reference
	 */
	UFUNCTION(BlueprintPure, Category = "Parser")
	UByteStreamPacketParser* GetParser() const { return Parser; }

	// === Statistics ===

	/** Get total packets decoded since creation/reset */
	UFUNCTION(BlueprintPure, Category = "Parser|Stats")
	int64 GetTotalPacketsDecoded() const;

	/** Get total bytes dropped since creation/reset */
	UFUNCTION(BlueprintPure, Category = "Parser|Stats")
	int64 GetTotalBytesDropped() const;

	/** Get total bad end frames since creation/reset */
	UFUNCTION(BlueprintPure, Category = "Parser|Stats")
	int64 GetTotalBadEndFrames() const;

	/**
	 * Reset all statistics counters
	 */
	UFUNCTION(BlueprintCallable, Category = "Parser|Stats")
	void ResetStatistics();

protected:
	/** Internal parser instance */
	UPROPERTY()
	UByteStreamPacketParser* Parser;

	/** Handler for parser packet decoded event */
	UFUNCTION()
	void HandlePacketDecoded(const FBenchPacket& Packet);

	/** Handler for parser bytes dropped event */
	UFUNCTION()
	void HandleBytesDropped(int32 ByteCount);

	/** Handler for parser bad end frame event */
	UFUNCTION()
	void HandleBadEndFrame();

	/** Initialize the parser with component settings */
	void InitializeParser();
};
