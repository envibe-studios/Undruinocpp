// Arduino Communication Plugin - Component Implementation

#include "ArduinoCommunicationComponent.h"

UArduinoCommunicationComponent::UArduinoCommunicationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SerialConnection = nullptr;
	TcpConnection = nullptr;
}

void UArduinoCommunicationComponent::BeginPlay()
{
	Super::BeginPlay();

	// Create connection objects
	SerialConnection = NewObject<UArduinoSerialPort>(this);
	TcpConnection = NewObject<UArduinoTcpClient>(this);

	// Setup event bindings
	SetupEventBindings();

	// Auto-connect if enabled
	if (bAutoConnect)
	{
		Connect();
	}
}

void UArduinoCommunicationComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Disconnect();
	Super::EndPlay(EndPlayReason);
}

void UArduinoCommunicationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UArduinoCommunicationComponent::SetupEventBindings()
{
	if (SerialConnection)
	{
		SerialConnection->OnDataReceived.AddDynamic(this, &UArduinoCommunicationComponent::HandleSerialDataReceived);
		SerialConnection->OnConnectionChanged.AddDynamic(this, &UArduinoCommunicationComponent::HandleSerialConnectionChanged);
		SerialConnection->OnError.AddDynamic(this, &UArduinoCommunicationComponent::HandleSerialError);
		SerialConnection->LineEnding = LineEnding;
	}

	if (TcpConnection)
	{
		TcpConnection->OnDataReceived.AddDynamic(this, &UArduinoCommunicationComponent::HandleTcpDataReceived);
		TcpConnection->OnConnectionChanged.AddDynamic(this, &UArduinoCommunicationComponent::HandleTcpConnectionChanged);
		TcpConnection->OnError.AddDynamic(this, &UArduinoCommunicationComponent::HandleTcpError);
		TcpConnection->LineEnding = LineEnding;
	}
}

bool UArduinoCommunicationComponent::Connect()
{
	switch (ConnectionMode)
	{
	case EArduinoConnectionMode::Serial:
		if (SerialConnection)
		{
			return SerialConnection->Open(SerialPort, BaudRate);
		}
		break;

	case EArduinoConnectionMode::TCP:
		if (TcpConnection)
		{
			return TcpConnection->Connect(IPAddress, TCPPort);
		}
		break;
	}

	return false;
}

void UArduinoCommunicationComponent::Disconnect()
{
	if (SerialConnection && SerialConnection->IsOpen())
	{
		SerialConnection->Close();
	}

	if (TcpConnection && TcpConnection->IsConnected())
	{
		TcpConnection->Disconnect();
	}
}

bool UArduinoCommunicationComponent::IsConnected() const
{
	switch (ConnectionMode)
	{
	case EArduinoConnectionMode::Serial:
		return SerialConnection && SerialConnection->IsOpen();

	case EArduinoConnectionMode::TCP:
		return TcpConnection && TcpConnection->IsConnected();
	}

	return false;
}

bool UArduinoCommunicationComponent::SendCommand(const FString& Command)
{
	switch (ConnectionMode)
	{
	case EArduinoConnectionMode::Serial:
		if (SerialConnection)
		{
			return SerialConnection->SendCommand(Command);
		}
		break;

	case EArduinoConnectionMode::TCP:
		if (TcpConnection)
		{
			return TcpConnection->SendCommand(Command);
		}
		break;
	}

	return false;
}

bool UArduinoCommunicationComponent::SendLine(const FString& Command)
{
	switch (ConnectionMode)
	{
	case EArduinoConnectionMode::Serial:
		if (SerialConnection)
		{
			return SerialConnection->SendLine(Command);
		}
		break;

	case EArduinoConnectionMode::TCP:
		if (TcpConnection)
		{
			return TcpConnection->SendLine(Command);
		}
		break;
	}

	return false;
}

TArray<FString> UArduinoCommunicationComponent::GetAvailablePorts()
{
	return UArduinoSerialPort::GetAvailablePorts();
}

void UArduinoCommunicationComponent::HandleSerialDataReceived(const FString& Data)
{
	OnDataReceived.Broadcast(Data);
}

void UArduinoCommunicationComponent::HandleSerialConnectionChanged(bool bConnected)
{
	OnConnectionChanged.Broadcast(bConnected);
}

void UArduinoCommunicationComponent::HandleSerialError(const FString& ErrorMessage)
{
	OnError.Broadcast(ErrorMessage);
}

void UArduinoCommunicationComponent::HandleTcpDataReceived(const FString& Data)
{
	OnDataReceived.Broadcast(Data);
}

void UArduinoCommunicationComponent::HandleTcpConnectionChanged(bool bConnected)
{
	OnConnectionChanged.Broadcast(bConnected);
}

void UArduinoCommunicationComponent::HandleTcpError(const FString& ErrorMessage)
{
	OnError.Broadcast(ErrorMessage);
}
