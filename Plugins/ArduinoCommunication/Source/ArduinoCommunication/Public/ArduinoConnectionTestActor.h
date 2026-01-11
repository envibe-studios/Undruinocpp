// Arduino Communication Plugin - Connection Test Actor
// A simple Blueprint-ready actor for testing ESP32 connections

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ArduinoCommunicationComponent.h"
#include "ArduinoConnectionTestActor.generated.h"

/**
 * Test result status for connection tests
 */
UENUM(BlueprintType)
enum class EArduinoTestStatus : uint8
{
	Idle			UMETA(DisplayName = "Idle"),
	Testing			UMETA(DisplayName = "Testing"),
	Success			UMETA(DisplayName = "Success"),
	Failed			UMETA(DisplayName = "Failed")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTestCompleted, bool, bSuccess, const FString&, Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTestStatusChanged, EArduinoTestStatus, NewStatus);

/**
 * Arduino Connection Test Actor
 *
 * A simple actor for testing ESP32/ESP8266 connections in Blueprints.
 * Place this actor in your level and configure the connection settings,
 * then call RunConnectionTest() to verify the connection works.
 *
 * Features:
 * - Automatic PING/PONG test
 * - LED toggle test
 * - Status query test
 * - Visual status feedback via Blueprint events
 */
UCLASS(Blueprintable, ClassGroup=(Arduino), meta=(DisplayName="Arduino Connection Test Actor"))
class ARDUINOCOMMUNICATION_API AArduinoConnectionTestActor : public AActor
{
	GENERATED_BODY()

public:
	AArduinoConnectionTestActor();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	// === Components ===

	/** The Arduino communication component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arduino|Components")
	UArduinoCommunicationComponent* ArduinoComponent;

	// === Test Settings ===

	/** Run test automatically when the game starts */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arduino|Test Settings")
	bool bAutoTestOnBeginPlay = false;

	/** Timeout for waiting for responses (in seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arduino|Test Settings", meta = (ClampMin = "1.0", ClampMax = "30.0"))
	float TestTimeoutSeconds = 5.0f;

	// === Status ===

	/** Current test status */
	UPROPERTY(BlueprintReadOnly, Category = "Arduino|Status")
	EArduinoTestStatus CurrentTestStatus = EArduinoTestStatus::Idle;

	/** Last test result message */
	UPROPERTY(BlueprintReadOnly, Category = "Arduino|Status")
	FString LastTestMessage;

	/** Number of successful tests */
	UPROPERTY(BlueprintReadOnly, Category = "Arduino|Status")
	int32 SuccessfulTests = 0;

	/** Number of failed tests */
	UPROPERTY(BlueprintReadOnly, Category = "Arduino|Status")
	int32 FailedTests = 0;

	// === Events ===

	/** Event fired when a test completes */
	UPROPERTY(BlueprintAssignable, Category = "Arduino|Events")
	FOnTestCompleted OnTestCompleted;

	/** Event fired when test status changes */
	UPROPERTY(BlueprintAssignable, Category = "Arduino|Events")
	FOnTestStatusChanged OnTestStatusChanged;

	// === Test Functions ===

	/**
	 * Run a complete connection test
	 * This will connect (if not connected), send PING, and verify PONG response
	 */
	UFUNCTION(BlueprintCallable, Category = "Arduino|Test")
	void RunConnectionTest();

	/**
	 * Run a PING test only (must be connected first)
	 * Sends PING and expects PONG response
	 */
	UFUNCTION(BlueprintCallable, Category = "Arduino|Test")
	void RunPingTest();

	/**
	 * Run an LED toggle test
	 * Sends LED_TOGGLE command and checks for OK response
	 */
	UFUNCTION(BlueprintCallable, Category = "Arduino|Test")
	void RunLedToggleTest();

	/**
	 * Run a status query test
	 * Sends STATUS command and checks for valid response
	 */
	UFUNCTION(BlueprintCallable, Category = "Arduino|Test")
	void RunStatusTest();

	/**
	 * Send a custom command and wait for response
	 * @param Command The command to send
	 */
	UFUNCTION(BlueprintCallable, Category = "Arduino|Test")
	void RunCustomCommandTest(const FString& Command);

	/**
	 * Cancel any running test
	 */
	UFUNCTION(BlueprintCallable, Category = "Arduino|Test")
	void CancelTest();

	/**
	 * Reset test statistics
	 */
	UFUNCTION(BlueprintCallable, Category = "Arduino|Test")
	void ResetTestStats();

	/**
	 * Get a human-readable status string
	 */
	UFUNCTION(BlueprintPure, Category = "Arduino|Test")
	FString GetStatusString() const;

	/**
	 * Check if the Arduino is currently connected
	 */
	UFUNCTION(BlueprintPure, Category = "Arduino|Test")
	bool IsConnected() const;

protected:
	/** Handle data received from Arduino */
	UFUNCTION()
	void HandleDataReceived(const FString& Data);

	/** Handle connection status changes */
	UFUNCTION()
	void HandleConnectionChanged(bool bConnected);

	/** Handle errors */
	UFUNCTION()
	void HandleError(const FString& ErrorMessage);

	/** Set the test status and broadcast event */
	void SetTestStatus(EArduinoTestStatus NewStatus);

	/** Complete the current test */
	void CompleteTest(bool bSuccess, const FString& Message);

	/** Handle test timeout */
	void OnTestTimeout();

private:
	/** Timer handle for test timeout */
	FTimerHandle TestTimeoutHandle;

	/** The command we're currently testing */
	FString CurrentTestCommand;

	/** Expected response pattern for current test */
	FString ExpectedResponse;

	/** Whether we're waiting for a connection before testing */
	bool bWaitingForConnection = false;
};
