// Arduino Communication Plugin - Serial Port Communication

#pragma once

#include "CoreMinimal.h"
#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"
#include "ArduinoSerialPort.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSerialDataReceived, const FString&, Data);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSerialConnectionChanged, bool, bConnected);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSerialError, const FString&, ErrorMessage);

/**
 * Serial Port Communication for Arduino ESP8266
 * Handles bidirectional text communication over COM ports
 */
UCLASS(BlueprintType, Blueprintable)
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

	/** Get list of available COM ports */
	UFUNCTION(BlueprintCallable, Category = "Arduino|Serial")
	static TArray<FString> GetAvailablePorts();

	/** Event fired when data is received from Arduino */
	UPROPERTY(BlueprintAssignable, Category = "Arduino|Serial|Events")
	FOnSerialDataReceived OnDataReceived;

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

	/** Thread-safe queue for received data */
	TQueue<FString> ReceivedDataQueue;

	/** Critical section for thread safety */
	FCriticalSection DataCriticalSection;

	/** Flag to stop the read thread */
	FThreadSafeBool bStopThread;

	/** Timer handle for processing received data */
	FTimerHandle ProcessTimerHandle;

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
