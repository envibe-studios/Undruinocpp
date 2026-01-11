// Arduino Communication Plugin - Connection Test Actor Implementation

#include "ArduinoConnectionTestActor.h"
#include "TimerManager.h"

AArduinoConnectionTestActor::AArduinoConnectionTestActor()
{
	PrimaryActorTick.bCanEverTick = false;

	// Create the Arduino communication component
	ArduinoComponent = CreateDefaultSubobject<UArduinoCommunicationComponent>(TEXT("ArduinoComponent"));
}

void AArduinoConnectionTestActor::BeginPlay()
{
	Super::BeginPlay();

	// Bind to Arduino component events
	if (ArduinoComponent)
	{
		ArduinoComponent->OnDataReceived.AddDynamic(this, &AArduinoConnectionTestActor::HandleDataReceived);
		ArduinoComponent->OnConnectionChanged.AddDynamic(this, &AArduinoConnectionTestActor::HandleConnectionChanged);
		ArduinoComponent->OnError.AddDynamic(this, &AArduinoConnectionTestActor::HandleError);
	}

	// Auto-test if enabled
	if (bAutoTestOnBeginPlay)
	{
		RunConnectionTest();
	}
}

void AArduinoConnectionTestActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Clear any pending timers
	GetWorldTimerManager().ClearTimer(TestTimeoutHandle);

	Super::EndPlay(EndPlayReason);
}

void AArduinoConnectionTestActor::RunConnectionTest()
{
	if (CurrentTestStatus == EArduinoTestStatus::Testing)
	{
		UE_LOG(LogTemp, Warning, TEXT("ArduinoConnectionTest: Test already in progress"));
		return;
	}

	SetTestStatus(EArduinoTestStatus::Testing);
	CurrentTestCommand = TEXT("PING");
	ExpectedResponse = TEXT("PONG");

	// Check if already connected
	if (ArduinoComponent && ArduinoComponent->IsConnected())
	{
		// Already connected, send PING directly
		bWaitingForConnection = false;
		ArduinoComponent->SendLine(TEXT("PING"));

		// Start timeout timer
		GetWorldTimerManager().SetTimer(
			TestTimeoutHandle,
			this,
			&AArduinoConnectionTestActor::OnTestTimeout,
			TestTimeoutSeconds,
			false
		);

		UE_LOG(LogTemp, Log, TEXT("ArduinoConnectionTest: Sent PING, waiting for PONG..."));
	}
	else if (ArduinoComponent)
	{
		// Need to connect first
		bWaitingForConnection = true;
		UE_LOG(LogTemp, Log, TEXT("ArduinoConnectionTest: Connecting..."));

		if (!ArduinoComponent->Connect())
		{
			CompleteTest(false, TEXT("Failed to initiate connection"));
		}
		else
		{
			// Start timeout timer for connection + response
			GetWorldTimerManager().SetTimer(
				TestTimeoutHandle,
				this,
				&AArduinoConnectionTestActor::OnTestTimeout,
				TestTimeoutSeconds,
				false
			);
		}
	}
	else
	{
		CompleteTest(false, TEXT("ArduinoComponent is not valid"));
	}
}

void AArduinoConnectionTestActor::RunPingTest()
{
	if (CurrentTestStatus == EArduinoTestStatus::Testing)
	{
		UE_LOG(LogTemp, Warning, TEXT("ArduinoConnectionTest: Test already in progress"));
		return;
	}

	if (!ArduinoComponent || !ArduinoComponent->IsConnected())
	{
		CompleteTest(false, TEXT("Not connected to Arduino"));
		return;
	}

	SetTestStatus(EArduinoTestStatus::Testing);
	CurrentTestCommand = TEXT("PING");
	ExpectedResponse = TEXT("PONG");
	bWaitingForConnection = false;

	ArduinoComponent->SendLine(TEXT("PING"));

	GetWorldTimerManager().SetTimer(
		TestTimeoutHandle,
		this,
		&AArduinoConnectionTestActor::OnTestTimeout,
		TestTimeoutSeconds,
		false
	);

	UE_LOG(LogTemp, Log, TEXT("ArduinoConnectionTest: Sent PING"));
}

void AArduinoConnectionTestActor::RunLedToggleTest()
{
	if (CurrentTestStatus == EArduinoTestStatus::Testing)
	{
		UE_LOG(LogTemp, Warning, TEXT("ArduinoConnectionTest: Test already in progress"));
		return;
	}

	if (!ArduinoComponent || !ArduinoComponent->IsConnected())
	{
		CompleteTest(false, TEXT("Not connected to Arduino"));
		return;
	}

	SetTestStatus(EArduinoTestStatus::Testing);
	CurrentTestCommand = TEXT("LED_TOGGLE");
	ExpectedResponse = TEXT("OK");
	bWaitingForConnection = false;

	ArduinoComponent->SendLine(TEXT("LED_TOGGLE"));

	GetWorldTimerManager().SetTimer(
		TestTimeoutHandle,
		this,
		&AArduinoConnectionTestActor::OnTestTimeout,
		TestTimeoutSeconds,
		false
	);

	UE_LOG(LogTemp, Log, TEXT("ArduinoConnectionTest: Sent LED_TOGGLE"));
}

void AArduinoConnectionTestActor::RunStatusTest()
{
	if (CurrentTestStatus == EArduinoTestStatus::Testing)
	{
		UE_LOG(LogTemp, Warning, TEXT("ArduinoConnectionTest: Test already in progress"));
		return;
	}

	if (!ArduinoComponent || !ArduinoComponent->IsConnected())
	{
		CompleteTest(false, TEXT("Not connected to Arduino"));
		return;
	}

	SetTestStatus(EArduinoTestStatus::Testing);
	CurrentTestCommand = TEXT("STATUS");
	ExpectedResponse = TEXT("STATUS");
	bWaitingForConnection = false;

	ArduinoComponent->SendLine(TEXT("STATUS"));

	GetWorldTimerManager().SetTimer(
		TestTimeoutHandle,
		this,
		&AArduinoConnectionTestActor::OnTestTimeout,
		TestTimeoutSeconds,
		false
	);

	UE_LOG(LogTemp, Log, TEXT("ArduinoConnectionTest: Sent STATUS"));
}

void AArduinoConnectionTestActor::RunCustomCommandTest(const FString& Command)
{
	if (CurrentTestStatus == EArduinoTestStatus::Testing)
	{
		UE_LOG(LogTemp, Warning, TEXT("ArduinoConnectionTest: Test already in progress"));
		return;
	}

	if (!ArduinoComponent || !ArduinoComponent->IsConnected())
	{
		CompleteTest(false, TEXT("Not connected to Arduino"));
		return;
	}

	SetTestStatus(EArduinoTestStatus::Testing);
	CurrentTestCommand = Command;
	ExpectedResponse = TEXT(""); // Accept any response
	bWaitingForConnection = false;

	ArduinoComponent->SendLine(Command);

	GetWorldTimerManager().SetTimer(
		TestTimeoutHandle,
		this,
		&AArduinoConnectionTestActor::OnTestTimeout,
		TestTimeoutSeconds,
		false
	);

	UE_LOG(LogTemp, Log, TEXT("ArduinoConnectionTest: Sent custom command: %s"), *Command);
}

void AArduinoConnectionTestActor::CancelTest()
{
	if (CurrentTestStatus == EArduinoTestStatus::Testing)
	{
		GetWorldTimerManager().ClearTimer(TestTimeoutHandle);
		SetTestStatus(EArduinoTestStatus::Idle);
		bWaitingForConnection = false;
		CurrentTestCommand.Empty();
		ExpectedResponse.Empty();
		UE_LOG(LogTemp, Log, TEXT("ArduinoConnectionTest: Test cancelled"));
	}
}

void AArduinoConnectionTestActor::ResetTestStats()
{
	SuccessfulTests = 0;
	FailedTests = 0;
}

FString AArduinoConnectionTestActor::GetStatusString() const
{
	switch (CurrentTestStatus)
	{
	case EArduinoTestStatus::Idle:
		return TEXT("Idle");
	case EArduinoTestStatus::Testing:
		return TEXT("Testing...");
	case EArduinoTestStatus::Success:
		return TEXT("Success");
	case EArduinoTestStatus::Failed:
		return TEXT("Failed");
	default:
		return TEXT("Unknown");
	}
}

bool AArduinoConnectionTestActor::IsConnected() const
{
	return ArduinoComponent && ArduinoComponent->IsConnected();
}

void AArduinoConnectionTestActor::HandleDataReceived(const FString& Data)
{
	if (CurrentTestStatus != EArduinoTestStatus::Testing)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("ArduinoConnectionTest: Received data: %s"), *Data);

	// Clear timeout timer
	GetWorldTimerManager().ClearTimer(TestTimeoutHandle);

	// Check if response matches expected
	FString TrimmedData = Data.TrimStartAndEnd();

	if (ExpectedResponse.IsEmpty())
	{
		// Any response is acceptable for custom commands
		CompleteTest(true, FString::Printf(TEXT("Received response: %s"), *TrimmedData));
	}
	else if (TrimmedData.Contains(ExpectedResponse))
	{
		CompleteTest(true, FString::Printf(TEXT("Test passed: %s -> %s"), *CurrentTestCommand, *TrimmedData));
	}
	else
	{
		CompleteTest(false, FString::Printf(TEXT("Unexpected response: expected '%s', got '%s'"), *ExpectedResponse, *TrimmedData));
	}
}

void AArduinoConnectionTestActor::HandleConnectionChanged(bool bConnected)
{
	UE_LOG(LogTemp, Log, TEXT("ArduinoConnectionTest: Connection changed: %s"), bConnected ? TEXT("Connected") : TEXT("Disconnected"));

	if (bWaitingForConnection && bConnected && CurrentTestStatus == EArduinoTestStatus::Testing)
	{
		// We were waiting for connection to send PING
		bWaitingForConnection = false;

		if (ArduinoComponent)
		{
			ArduinoComponent->SendLine(CurrentTestCommand);
			UE_LOG(LogTemp, Log, TEXT("ArduinoConnectionTest: Connected, sent %s"), *CurrentTestCommand);
		}
	}
	else if (bWaitingForConnection && !bConnected && CurrentTestStatus == EArduinoTestStatus::Testing)
	{
		// Connection failed while waiting
		GetWorldTimerManager().ClearTimer(TestTimeoutHandle);
		CompleteTest(false, TEXT("Connection failed"));
	}
}

void AArduinoConnectionTestActor::HandleError(const FString& ErrorMessage)
{
	UE_LOG(LogTemp, Error, TEXT("ArduinoConnectionTest: Error: %s"), *ErrorMessage);

	if (CurrentTestStatus == EArduinoTestStatus::Testing)
	{
		GetWorldTimerManager().ClearTimer(TestTimeoutHandle);
		CompleteTest(false, FString::Printf(TEXT("Error: %s"), *ErrorMessage));
	}
}

void AArduinoConnectionTestActor::SetTestStatus(EArduinoTestStatus NewStatus)
{
	if (CurrentTestStatus != NewStatus)
	{
		CurrentTestStatus = NewStatus;
		OnTestStatusChanged.Broadcast(NewStatus);
	}
}

void AArduinoConnectionTestActor::CompleteTest(bool bSuccess, const FString& Message)
{
	LastTestMessage = Message;

	if (bSuccess)
	{
		SuccessfulTests++;
		SetTestStatus(EArduinoTestStatus::Success);
		UE_LOG(LogTemp, Log, TEXT("ArduinoConnectionTest: SUCCESS - %s"), *Message);
	}
	else
	{
		FailedTests++;
		SetTestStatus(EArduinoTestStatus::Failed);
		UE_LOG(LogTemp, Warning, TEXT("ArduinoConnectionTest: FAILED - %s"), *Message);
	}

	bWaitingForConnection = false;
	CurrentTestCommand.Empty();
	ExpectedResponse.Empty();

	OnTestCompleted.Broadcast(bSuccess, Message);
}

void AArduinoConnectionTestActor::OnTestTimeout()
{
	if (CurrentTestStatus == EArduinoTestStatus::Testing)
	{
		CompleteTest(false, FString::Printf(TEXT("Test timed out after %.1f seconds"), TestTimeoutSeconds));
	}
}
