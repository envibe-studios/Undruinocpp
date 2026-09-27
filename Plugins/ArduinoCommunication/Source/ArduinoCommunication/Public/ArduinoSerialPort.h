// Arduino Communication Plugin - Serial Port Communication

#pragma once

#include "CoreMinimal.h"
#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"
#include "ArduinoSerialPort.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSerialDataReceived, const FString&, Data);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSerialByteReceived, const TArray<uint8>&, Bytes);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSerialLineReceived, const FString&, Line);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSerialConnectionChanged, bool, bConnected);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSerialError, const FString&, ErrorMessage);

/**
 * Serial Port Communication for Arduino ESP8266
 * Handles bidirectional text communication over COM ports
 */
UCLASS(BlueprintType, Blueprintable, Config=Game)
class ARDUINOCOMMUNICATION_API UArduinoSerialPort : public UObject
{
	GENERATED_BODY()

public:
	UArduinoSerialPort();
	virtual ~UArduinoSerialPort();

	/** Initialize and open the serial port */
	UFUNCTION(BlueprintCallable, Category = "Arduino|Serial")
	bool Open(const FString& PortName, int32 BaudRate = 115200);

	/** Close the serial port connection */
	UFUNCTION(BlueprintCallable, Category = "Arduino|Serial")
	void Close();

	/** Check if the serial port is currently open */
	UFUNCTION(BlueprintPure, Category = "Arduino|Serial")
	bool IsOpen() const;

	/** Send a text command to the Arduino */
	UFUNCTION(BlueprintCallable, Category = "Arduino|Serial")
	bool SendCommand(const FString& Command);

	/** Send a text command with newline terminator */
	UFUNCTION(BlueprintCallable, Category = "Arduino|Serial")
	bool SendLine(const FString& Command);

	/** Write an ASCII line with explicit \n terminator (for commands like !jack_start\n) */
	UFUNCTION(BlueprintCallable, Category = "Arduino|Serial")
	bool WriteAsciiLine(const FString& Line);

	/** Get list of available COM ports */
	UFUNCTION(BlueprintCallable, Category = "Arduino|Serial")
	static TArray<FString> GetAvailablePorts();

	/** Event fired when data is received from Arduino (legacy, same as OnLineReceived) */
	UPROPERTY(BlueprintAssignable, Category = "Arduino|Serial|Events")
	FOnSerialDataReceived OnDataReceived;

	/** Event fired when raw bytes are received from Arduino (before line parsing) */
	UPROPERTY(BlueprintAssignable, Category = "Arduino|Serial|Events")
	FOnSerialByteReceived OnByteReceived;

	/** Event fired when a complete line is received from Arduino (parsed by LineEnding) */
	UPROPERTY(BlueprintAssignable, Category = "Arduino|Serial|Events")
	FOnSerialLineReceived OnLineReceived;

	/** Event fired when connection status changes */
	UPROPERTY(BlueprintAssignable, Category = "Arduino|Serial|Events")
	FOnSerialConnectionChanged OnConnectionChanged;

	/** Event fired when an error occurs */
	UPROPERTY(BlueprintAssignable, Category = "Arduino|Serial|Events")
	FOnSerialError OnError;

	/** Line ending mode for received data */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arduino|Serial")
	FString LineEnding = TEXT("\n");

	/** Read buffer size */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arduino|Serial")
	int32 BufferSize = 4096;

	// ============================================================
	// RAW TAP DIAGNOSTICS - Debug serial byte flow before parsing
	// ============================================================

	/** Enable hex dump of raw serial bytes (up to first 32 bytes per read) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category = "Arduino|Serial|RawTap")
	bool bDumpRawSerial = false;

	/** Bypass the line parser entirely - only show raw hex dumps + counters */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category = "Arduino|Serial|RawTap")
	bool bBypassParser = false;

	/** Enable on-screen debug display of raw tap statistics */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category = "Arduino|Serial|RawTap")
	bool bShowRawTapOnScreen = false;

	/** Enable poll mode fallback (reads on game thread timer instead of worker thread) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category = "Arduino|Serial|RawTap")
	bool bUsePollMode = false;

	/** Enable verbose diagnostic logging for all serial operations */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category = "Arduino|Serial|RawTap")
	bool bVerboseDiagnostics = false;

	/** Set all raw tap options at once */
	UFUNCTION(BlueprintCallable, Category = "Arduino|Serial|RawTap")
	void SetRawTapOptions(bool bDump, bool bBypass, bool bOnScreen);

	/** Set serial raw tap options including poll mode */
	UFUNCTION(BlueprintCallable, Category = "Arduino|Serial|RawTap")
	void SetSerialRawTapOptions(bool bDump, bool bBypass, bool bOnScreen, bool bPollMode, bool bVerbose);

	/** Total bytes read from serial port since connection opened */
	UPROPERTY(BlueprintReadOnly, Category = "Arduino|Serial|RawTap")
	int64 BytesReadTotal = 0;

	/** Total number of read operations since connection opened */
	UPROPERTY(BlueprintReadOnly, Category = "Arduino|Serial|RawTap")
	int64 ReadsCount = 0;

	/** Size of the last successful read operation */
	UPROPERTY(BlueprintReadOnly, Category = "Arduino|Serial|RawTap")
	int32 LastReadSize = 0;

	/** Number of 0xAA start bytes detected in raw stream */
	UPROPERTY(BlueprintReadOnly, Category = "Arduino|Serial|RawTap")
	int64 StartByteHits = 0;

	/** Timestamp of last byte received (FPlatformTime::Seconds) */
	UPROPERTY(BlueprintReadOnly, Category = "Arduino|Serial|RawTap")
	double LastByteTime = 0.0;

	/** Get raw tap statistics as formatted string */
	UFUNCTION(BlueprintCallable, Category = "Arduino|Serial|RawTap")
	FString GetRawTapStats() const;

	/** Reset raw tap counters to zero */
	UFUNCTION(BlueprintCallable, Category = "Arduino|Serial|RawTap")
	void ResetRawTapCounters();

	/** Number of zero-byte reads (read returned 0 bytes) */
	UPROPERTY(BlueprintReadOnly, Category = "Arduino|Serial|RawTap")
	int64 ZeroByteReads = 0;

	/** Last error code from read operation (0 = success) */
	UPROPERTY(BlueprintReadOnly, Category = "Arduino|Serial|RawTap")
	int32 LastReadError = 0;

protected:
	/** Process incoming data on the game thread */
	void ProcessReceivedData();

	/** Start the read thread */
	void StartReadThread();

	/** Stop the read thread */
	void StopReadThread();

private:
	/** Platform-specific serial port handle */
	void* SerialHandle;

	/** Flag indicating if port is open */
	bool bIsOpen;

	/** Current port name */
	FString CurrentPortName;

	/** Current baud rate */
	int32 CurrentBaudRate;

	/** Thread for reading data */
	FRunnableThread* ReadThread;

	/** Runnable for read thread */
	class FSerialReadRunnable* ReadRunnable;

	/** Buffer for incomplete lines */
	FString ReceiveBuffer;

	/** Thread-safe queue for received lines */
	TQueue<FString> ReceivedDataQueue;

	/** Thread-safe queue for received raw bytes */
	TQueue<TArray<uint8>> ReceivedBytesQueue;

	/** Critical section for thread safety */
	FCriticalSection DataCriticalSection;

	/** Flag to stop the read thread */
	FThreadSafeBool bStopThread;

	/** Timer handle for processing received data */
	FTimerHandle ProcessTimerHandle;

	/** Timer handle for poll mode reading */
	FTimerHandle PollTimerHandle;

	/** Timer handle for unconditional 1-second stats logging */
	FTimerHandle StatsTimerHandle;

	/** Critical section for raw tap counter thread safety */
	FCriticalSection RawTapCriticalSection;

	/** Poll mode read function (called from game thread timer) */
	void PollRead();

	/** Update and display on-screen debug stats (called from game thread) */
	void UpdateOnScreenDebug();

	/** Log periodic stats unconditionally (called once per second from game thread) */
	void LogPeriodicStats();

	/** Process raw tap diagnostics for a read chunk (called from read thread) */
	void ProcessRawTap(const uint8* Buffer, int32 BytesRead);

	/** Format bytes as hex dump string */
	static FString FormatHexDump(const uint8* Buffer, int32 BytesRead, int32 MaxBytes = 32);

	/** Format bytes as ASCII view (non-printables as '.') */
	static FString FormatAsciiView(const uint8* Buffer, int32 BytesRead, int32 MaxBytes = 32);

	friend class FSerialReadRunnable;
};

/**
 * Runnable class for reading serial data in background
 */
class FSerialReadRunnable : public FRunnable
{
public:
	FSerialReadRunnable(UArduinoSerialPort* InOwner);
	virtual ~FSerialReadRunnable();

	// FRunnable interface
	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Stop() override;
	virtual void Exit() override;

private:
	UArduinoSerialPort* Owner;
	FThreadSafeBool bRunning;
};
