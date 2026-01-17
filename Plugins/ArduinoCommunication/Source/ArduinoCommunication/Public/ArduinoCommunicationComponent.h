// Arduino Communication Plugin - Actor Component

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ArduinoSerialPort.h"
#include "ArduinoTcpClient.h"
#include "ArduinoCommunicationComponent.generated.h"

/**
 * Communication mode selection
 */
UENUM(BlueprintType)
enum class EArduinoConnectionMode : uint8
{
	Serial		UMETA(DisplayName = "Serial (USB)"),
	TCP			UMETA(DisplayName = "TCP/WiFi")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnArduinoDataReceived, const FString&, Data);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnArduinoByteReceived, const TArray<uint8>&, Bytes);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnArduinoLineReceived, const FString&, Line);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnArduinoConnectionChanged, bool, bConnected);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnArduinoError, const FString&, ErrorMessage);

/**
 * Arduino Communication Component
 * Easy-to-use component for communicating with Arduino ESP8266
 * Supports both Serial (USB) and TCP (WiFi) connections
 */
UCLASS(ClassGroup=(Arduino), meta=(BlueprintSpawnableComponent))
class ARDUINOCOMMUNICATION_API UArduinoCommunicationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UArduinoCommunicationComponent();

	// UActorComponent interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Connection mode (Serial or TCP) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arduino|Connection")
	EArduinoConnectionMode ConnectionMode = EArduinoConnectionMode::Serial;

	/** Auto-connect on BeginPlay */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arduino|Connection")
	bool bAutoConnect = false;

	// === Serial Settings ===

	/** COM port name (e.g., "COM3") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arduino|Serial", meta = (EditCondition = "ConnectionMode == EArduinoConnectionMode::Serial"))
	FString SerialPort = TEXT("COM3");

	/** Serial baud rate */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arduino|Serial", meta = (EditCondition = "ConnectionMode == EArduinoConnectionMode::Serial"))
	int32 BaudRate = 115200;

	// === TCP Settings ===

	/** Arduino IP address for TCP connection */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arduino|TCP", meta = (EditCondition = "ConnectionMode == EArduinoConnectionMode::TCP"))
	FString IPAddress = TEXT("192.168.1.100");

	/** TCP port number */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arduino|TCP", meta = (EditCondition = "ConnectionMode == EArduinoConnectionMode::TCP"))
	int32 TCPPort = 80;

	// === Common Settings ===

	/** Line ending for messages */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arduino|Settings")
	FString LineEnding = TEXT("\n");

	// === Raw Tap Diagnostics (Serial Only) ===

	/** Enable hex dump of raw serial bytes (up to first 32 bytes per read) - Serial mode only */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arduino|Serial|RawTap", meta = (EditCondition = "ConnectionMode == EArduinoConnectionMode::Serial"))
	bool bDumpRawSerial = false;

	/** Bypass the line parser entirely - only show raw hex dumps + counters - Serial mode only */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arduino|Serial|RawTap", meta = (EditCondition = "ConnectionMode == EArduinoConnectionMode::Serial"))
	bool bBypassParser = false;

	/** Enable on-screen debug display of raw tap statistics - Serial mode only */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arduino|Serial|RawTap", meta = (EditCondition = "ConnectionMode == EArduinoConnectionMode::Serial"))
	bool bShowRawTapOnScreen = false;

	// === Events ===

	/** Event fired when data is received from Arduino (legacy, same as OnLineReceived) */
	UPROPERTY(BlueprintAssignable, Category = "Arduino|Events")
	FOnArduinoDataReceived OnDataReceived;

	/** Event fired when raw bytes are received from Arduino (before line parsing) */
	UPROPERTY(BlueprintAssignable, Category = "Arduino|Events")
	FOnArduinoByteReceived OnByteReceived;

	/** Event fired when a complete line is received from Arduino (parsed by LineEnding) */
	UPROPERTY(BlueprintAssignable, Category = "Arduino|Events")
	FOnArduinoLineReceived OnLineReceived;

	/** Event fired when connection status changes */
	UPROPERTY(BlueprintAssignable, Category = "Arduino|Events")
	FOnArduinoConnectionChanged OnConnectionChanged;

	/** Event fired when an error occurs */
	UPROPERTY(BlueprintAssignable, Category = "Arduino|Events")
	FOnArduinoError OnError;

	// === Functions ===

	/** Connect to Arduino using current settings */
	UFUNCTION(BlueprintCallable, Category = "Arduino")
	bool Connect();

	/** Disconnect from Arduino */
	UFUNCTION(BlueprintCallable, Category = "Arduino")
	void Disconnect();

	/** Check if currently connected */
	UFUNCTION(BlueprintPure, Category = "Arduino")
	bool IsConnected() const;

	/** Send a text command to Arduino */
	UFUNCTION(BlueprintCallable, Category = "Arduino")
	bool SendCommand(const FString& Command);

	/** Send a text command with line ending */
	UFUNCTION(BlueprintCallable, Category = "Arduino")
	bool SendLine(const FString& Command);

	/** Get available serial ports */
	UFUNCTION(BlueprintCallable, Category = "Arduino|Serial")
	TArray<FString> GetAvailablePorts();

	// === Raw Tap Functions (Serial Only) ===

	/** Set raw tap diagnostic options for serial connection */
	UFUNCTION(BlueprintCallable, Category = "Arduino|Serial|RawTap")
	void SetSerialRawTapOptions(bool bDump, bool bBypass, bool bOnScreen);

	/** Reset raw tap counters to zero */
	UFUNCTION(BlueprintCallable, Category = "Arduino|Serial|RawTap")
	void ResetSerialRawTapCounters();

	/** Get raw tap statistics as formatted string */
	UFUNCTION(BlueprintCallable, Category = "Arduino|Serial|RawTap")
	FString GetSerialRawTapStats() const;

	/** Get direct access to the serial port object (advanced use) */
	UFUNCTION(BlueprintCallable, Category = "Arduino|Serial")
	UArduinoSerialPort* GetSerialPort() const { return SerialConnection; }

protected:
	/** Handle data received from serial port */
	UFUNCTION()
	void HandleSerialDataReceived(const FString& Data);

	UFUNCTION()
	void HandleSerialByteReceived(const TArray<uint8>& Bytes);

	UFUNCTION()
	void HandleSerialLineReceived(const FString& Line);

	UFUNCTION()
	void HandleSerialConnectionChanged(bool bConnected);

	UFUNCTION()
	void HandleSerialError(const FString& ErrorMessage);

	/** Handle data received from TCP */
	UFUNCTION()
	void HandleTcpDataReceived(const FString& Data);

	UFUNCTION()
	void HandleTcpByteReceived(const TArray<uint8>& Bytes);

	UFUNCTION()
	void HandleTcpLineReceived(const FString& Line);

	UFUNCTION()
	void HandleTcpConnectionChanged(bool bConnected);

	UFUNCTION()
	void HandleTcpError(const FString& ErrorMessage);

private:
	/** Serial port instance */
	UPROPERTY()
	UArduinoSerialPort* SerialConnection;

	/** TCP client instance */
	UPROPERTY()
	UArduinoTcpClient* TcpConnection;

	/** Setup event bindings */
	void SetupEventBindings();
};
