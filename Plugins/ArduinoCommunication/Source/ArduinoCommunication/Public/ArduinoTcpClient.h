// Arduino Communication Plugin - TCP/WiFi Communication

#pragma once

#include "CoreMinimal.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Networking.h"
#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"
#include "ArduinoTcpClient.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTcpDataReceived, const FString&, Data);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTcpConnectionChanged, bool, bConnected);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTcpError, const FString&, ErrorMessage);

/**
 * TCP Client for WiFi communication with Arduino ESP8266
 * Enables wireless bidirectional text communication
 */
UCLASS(BlueprintType, Blueprintable)
class ARDUINOCOMMUNICATION_API UArduinoTcpClient : public UObject
{
	GENERATED_BODY()

public:
	UArduinoTcpClient();
	virtual ~UArduinoTcpClient();

	/** Connect to Arduino ESP8266 via TCP/WiFi */
	UFUNCTION(BlueprintCallable, Category = "Arduino|TCP")
	bool Connect(const FString& IPAddress, int32 Port = 80);

	/** Disconnect from the Arduino */
	UFUNCTION(BlueprintCallable, Category = "Arduino|TCP")
	void Disconnect();

	/** Check if currently connected */
	UFUNCTION(BlueprintPure, Category = "Arduino|TCP")
	bool IsConnected() const;

	/** Send a text command to the Arduino */
	UFUNCTION(BlueprintCallable, Category = "Arduino|TCP")
	bool SendCommand(const FString& Command);

	/** Send a text command with newline terminator */
	UFUNCTION(BlueprintCallable, Category = "Arduino|TCP")
	bool SendLine(const FString& Command);

	/** Get the current connection IP address */
	UFUNCTION(BlueprintPure, Category = "Arduino|TCP")
	FString GetIPAddress() const { return CurrentIPAddress; }

	/** Get the current connection port */
	UFUNCTION(BlueprintPure, Category = "Arduino|TCP")
	int32 GetPort() const { return CurrentPort; }

	/** Event fired when data is received from Arduino */
	UPROPERTY(BlueprintAssignable, Category = "Arduino|TCP|Events")
	FOnTcpDataReceived OnDataReceived;

	/** Event fired when connection status changes */
	UPROPERTY(BlueprintAssignable, Category = "Arduino|TCP|Events")
	FOnTcpConnectionChanged OnConnectionChanged;

	/** Event fired when an error occurs */
	UPROPERTY(BlueprintAssignable, Category = "Arduino|TCP|Events")
	FOnTcpError OnError;

	/** Line ending mode for received data */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arduino|TCP")
	FString LineEnding = TEXT("\n");

	/** Connection timeout in seconds */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arduino|TCP")
	float ConnectionTimeout = 5.0f;

	/** Receive buffer size */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arduino|TCP")
	int32 BufferSize = 4096;

protected:
	/** Process incoming data on the game thread */
	void ProcessReceivedData();

	/** Start the receive thread */
	void StartReceiveThread();

	/** Stop the receive thread */
	void StopReceiveThread();

private:
	/** Socket for TCP connection */
	FSocket* Socket;

	/** Flag indicating if connected */
	bool bIsConnected;

	/** Current IP address */
	FString CurrentIPAddress;

	/** Current port */
	int32 CurrentPort;

	/** Thread for receiving data */
	FRunnableThread* ReceiveThread;

	/** Runnable for receive thread */
	class FTcpReceiveRunnable* ReceiveRunnable;

	/** Buffer for incomplete lines */
	FString ReceiveBuffer;

	/** Thread-safe queue for received data */
	TQueue<FString> ReceivedDataQueue;

	/** Critical section for thread safety */
	FCriticalSection DataCriticalSection;

	/** Flag to stop the receive thread */
	FThreadSafeBool bStopThread;

	/** Timer handle for processing received data */
	FTimerHandle ProcessTimerHandle;

	friend class FTcpReceiveRunnable;
};

/**
 * Runnable class for receiving TCP data in background
 */
class FTcpReceiveRunnable : public FRunnable
{
public:
	FTcpReceiveRunnable(UArduinoTcpClient* InOwner);
	virtual ~FTcpReceiveRunnable();

	// FRunnable interface
	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Stop() override;
	virtual void Exit() override;

private:
	UArduinoTcpClient* Owner;
	FThreadSafeBool bRunning;
};
