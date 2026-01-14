// Arduino Communication Plugin - TCP Client Implementation

#include "ArduinoTcpClient.h"
#include "Async/Async.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Common/TcpSocketBuilder.h"
#include "Interfaces/IPv4/IPv4Address.h"

UArduinoTcpClient::UArduinoTcpClient()
	: Socket(nullptr)
	, bIsConnected(false)
	, CurrentPort(80)
	, ReceiveThread(nullptr)
	, ReceiveRunnable(nullptr)
	, bStopThread(false)
{
}

UArduinoTcpClient::~UArduinoTcpClient()
{
	Disconnect();
}

bool UArduinoTcpClient::Connect(const FString& IPAddress, int32 Port)
{
	if (bIsConnected)
	{
		Disconnect();
	}

	// Parse IP address
	FIPv4Address IP;
	if (!FIPv4Address::Parse(IPAddress, IP))
	{
		FString ErrorMsg = FString::Printf(TEXT("Invalid IP address: %s"), *IPAddress);
		UE_LOG(LogTemp, Error, TEXT("ArduinoTcp: %s"), *ErrorMsg);
		OnError.Broadcast(ErrorMsg);
		return false;
	}

	// Create socket
	ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	Socket = SocketSubsystem->CreateSocket(NAME_Stream, TEXT("ArduinoTcpSocket"), false);

	if (Socket == nullptr)
	{
		FString ErrorMsg = TEXT("Failed to create socket");
		UE_LOG(LogTemp, Error, TEXT("ArduinoTcp: %s"), *ErrorMsg);
		OnError.Broadcast(ErrorMsg);
		return false;
	}

	// Set socket options
	Socket->SetNonBlocking(false);
	Socket->SetNoDelay(true);

	// Create endpoint
	TSharedRef<FInternetAddr> Addr = SocketSubsystem->CreateInternetAddr();
	Addr->SetIp(IP.Value);
	Addr->SetPort(Port);

	// Connect with timeout
	UE_LOG(LogTemp, Log, TEXT("ArduinoTcp: Connecting to %s:%d..."), *IPAddress, Port);

	if (!Socket->Connect(*Addr))
	{
		FString ErrorMsg = FString::Printf(TEXT("Failed to connect to %s:%d"), *IPAddress, Port);
		UE_LOG(LogTemp, Error, TEXT("ArduinoTcp: %s"), *ErrorMsg);
		OnError.Broadcast(ErrorMsg);

		Socket->Close();
		SocketSubsystem->DestroySocket(Socket);
		Socket = nullptr;
		return false;
	}

	// Verify connection
	ESocketConnectionState State = Socket->GetConnectionState();
	if (State != SCS_Connected)
	{
		FString ErrorMsg = FString::Printf(TEXT("Connection failed. State: %d"), (int32)State);
		UE_LOG(LogTemp, Error, TEXT("ArduinoTcp: %s"), *ErrorMsg);
		OnError.Broadcast(ErrorMsg);

		Socket->Close();
		SocketSubsystem->DestroySocket(Socket);
		Socket = nullptr;
		return false;
	}

	bIsConnected = true;
	CurrentIPAddress = IPAddress;
	CurrentPort = Port;

	UE_LOG(LogTemp, Log, TEXT("ArduinoTcp: Connected to %s:%d"), *IPAddress, Port);

	// Start receive thread
	StartReceiveThread();

	// Broadcast connection event
	OnConnectionChanged.Broadcast(true);

	return true;
}

void UArduinoTcpClient::Disconnect()
{
	if (!bIsConnected)
	{
		return;
	}

	// Stop receive thread first
	StopReceiveThread();

	if (Socket != nullptr)
	{
		Socket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Socket);
		Socket = nullptr;
	}

	bIsConnected = false;
	ReceiveBuffer.Empty();

	UE_LOG(LogTemp, Log, TEXT("ArduinoTcp: Disconnected from %s:%d"), *CurrentIPAddress, CurrentPort);

	// Broadcast disconnection event
	OnConnectionChanged.Broadcast(false);
}

bool UArduinoTcpClient::IsConnected() const
{
	if (!bIsConnected || Socket == nullptr)
	{
		return false;
	}

	return Socket->GetConnectionState() == SCS_Connected;
}

bool UArduinoTcpClient::SendCommand(const FString& Command)
{
	if (!bIsConnected || Socket == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("ArduinoTcp: Cannot send - not connected"));
		return false;
	}

	// Convert to UTF-8
	FTCHARToUTF8 Converter(*Command);
	const uint8* Data = (const uint8*)Converter.Get();
	int32 DataLength = Converter.Length();

	int32 BytesSent = 0;
	if (!Socket->Send(Data, DataLength, BytesSent))
	{
		FString ErrorMsg = TEXT("Failed to send data");
		UE_LOG(LogTemp, Error, TEXT("ArduinoTcp: %s"), *ErrorMsg);
		OnError.Broadcast(ErrorMsg);
		return false;
	}

	if (BytesSent != DataLength)
	{
		FString ErrorMsg = FString::Printf(TEXT("Incomplete send. Sent %d of %d bytes"), BytesSent, DataLength);
		UE_LOG(LogTemp, Warning, TEXT("ArduinoTcp: %s"), *ErrorMsg);
	}

	UE_LOG(LogTemp, Verbose, TEXT("ArduinoTcp: Sent: %s"), *Command);
	return true;
}

bool UArduinoTcpClient::SendLine(const FString& Command)
{
	return SendCommand(Command + LineEnding);
}

void UArduinoTcpClient::StartReceiveThread()
{
	bStopThread = false;
	ReceiveRunnable = new FTcpReceiveRunnable(this);
	ReceiveThread = FRunnableThread::Create(ReceiveRunnable, TEXT("ArduinoTcpReceiveThread"));

	// Set up timer to process received data on game thread
	if (GEngine && GEngine->GetWorld())
	{
		GEngine->GetWorld()->GetTimerManager().SetTimer(
			ProcessTimerHandle,
			this,
			&UArduinoTcpClient::ProcessReceivedData,
			0.016f, // ~60 FPS
			true
		);
	}
}

void UArduinoTcpClient::StopReceiveThread()
{
	bStopThread = true;

	// Clear timer
	if (GEngine && GEngine->GetWorld())
	{
		GEngine->GetWorld()->GetTimerManager().ClearTimer(ProcessTimerHandle);
	}

	if (ReceiveThread != nullptr)
	{
		ReceiveThread->WaitForCompletion();
		delete ReceiveThread;
		ReceiveThread = nullptr;
	}

	if (ReceiveRunnable != nullptr)
	{
		delete ReceiveRunnable;
		ReceiveRunnable = nullptr;
	}
}

void UArduinoTcpClient::ProcessReceivedData()
{
	// Process raw bytes
	TArray<uint8> Bytes;
	while (ReceivedBytesQueue.Dequeue(Bytes))
	{
		OnByteReceived.Broadcast(Bytes);
	}

	// Process complete lines
	FString Data;
	while (ReceivedDataQueue.Dequeue(Data))
	{
		OnDataReceived.Broadcast(Data);
		OnLineReceived.Broadcast(Data);
	}
}

// FTcpReceiveRunnable implementation

FTcpReceiveRunnable::FTcpReceiveRunnable(UArduinoTcpClient* InOwner)
	: Owner(InOwner)
	, bRunning(false)
{
}

FTcpReceiveRunnable::~FTcpReceiveRunnable()
{
}

bool FTcpReceiveRunnable::Init()
{
	bRunning = true;
	return true;
}

uint32 FTcpReceiveRunnable::Run()
{
	uint8 ReadBuffer[256];

	while (bRunning && Owner && Owner->bIsConnected && !Owner->bStopThread)
	{
		FSocket* Socket = Owner->Socket;
		if (Socket == nullptr)
		{
			break;
		}

		// Check if data is available
		uint32 PendingDataSize = 0;
		if (Socket->HasPendingData(PendingDataSize) && PendingDataSize > 0)
		{
			int32 BytesRead = 0;
			if (Socket->Recv(ReadBuffer, FMath::Min((int32)PendingDataSize, (int32)sizeof(ReadBuffer) - 1), BytesRead))
			{
				if (BytesRead > 0)
				{
					// Enqueue raw bytes for OnByteReceived
					TArray<uint8> RawBytes;
					RawBytes.Append(ReadBuffer, BytesRead);
					Owner->ReceivedBytesQueue.Enqueue(RawBytes);

					ReadBuffer[BytesRead] = '\0';

					// Convert from UTF-8 to FString
					FUTF8ToTCHAR Converter((const char*)ReadBuffer, BytesRead);
					FString ReceivedText(Converter.Length(), Converter.Get());

					// Add to buffer
					Owner->ReceiveBuffer += ReceivedText;

					// Process complete lines
					FString Line;
					while (Owner->ReceiveBuffer.Split(Owner->LineEnding, &Line, &Owner->ReceiveBuffer))
					{
						if (!Line.IsEmpty())
						{
							Owner->ReceivedDataQueue.Enqueue(Line);
						}
					}
				}
			}
		}

		// Small sleep to prevent busy waiting
		FPlatformProcess::Sleep(0.001f);
	}

	return 0;
}

void FTcpReceiveRunnable::Stop()
{
	bRunning = false;
}

void FTcpReceiveRunnable::Exit()
{
	bRunning = false;
}
