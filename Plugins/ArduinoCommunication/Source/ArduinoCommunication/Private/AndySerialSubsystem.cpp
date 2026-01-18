// Arduino Communication Plugin - Andy Serial Subsystem Implementation

#include "AndySerialSubsystem.h"
#include "ArduinoSerialPort.h"
#include "ByteStreamPacketParser.h"
#include "Engine/GameInstance.h"
#include "Async/Async.h"

// ============================================================================
// UAndyPortEventHandler Implementation
// ============================================================================

void UAndyPortEventHandler::Setup(UAndySerialSubsystem* InOwner, FName InShipId)
{
	OwnerSubsystem = InOwner;
	ShipId = InShipId;
}

void UAndyPortEventHandler::BindToPort(UArduinoSerialPort* Port)
{
	if (!Port)
	{
		return;
	}

	Port->OnByteReceived.AddDynamic(this, &UAndyPortEventHandler::OnBytesReceived);
	Port->OnConnectionChanged.AddDynamic(this, &UAndyPortEventHandler::OnConnectionChanged);
}

void UAndyPortEventHandler::UnbindFromPort(UArduinoSerialPort* Port)
{
	if (!Port)
	{
		return;
	}

	Port->OnByteReceived.RemoveDynamic(this, &UAndyPortEventHandler::OnBytesReceived);
	Port->OnConnectionChanged.RemoveDynamic(this, &UAndyPortEventHandler::OnConnectionChanged);
}

void UAndyPortEventHandler::OnBytesReceived(const TArray<uint8>& Bytes)
{
	if (OwnerSubsystem)
	{
		OwnerSubsystem->HandleBytesReceived(ShipId, Bytes);
	}
}

void UAndyPortEventHandler::OnConnectionChanged(bool bConnected)
{
	if (OwnerSubsystem)
	{
		OwnerSubsystem->HandleConnectionChanged(ShipId, bConnected);
	}
}

// ============================================================================
// UAndySerialSubsystem Implementation
// ============================================================================

UAndySerialSubsystem::UAndySerialSubsystem()
{
}

void UAndySerialSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UE_LOG(LogTemp, Log, TEXT("AndySerialSubsystem: Initialized"));
}

void UAndySerialSubsystem::Deinitialize()
{
	// Stop and clean up all connections
	StopAll();
	Connections.Empty();

	UE_LOG(LogTemp, Log, TEXT("AndySerialSubsystem: Deinitialized"));

	Super::Deinitialize();
}

bool UAndySerialSubsystem::AddPort(FName ShipId, const FString& PortName, int32 BaudRate)
{
	// Check if ShipId already exists
	if (Connections.Contains(ShipId))
	{
		UE_LOG(LogTemp, Warning, TEXT("AndySerialSubsystem: ShipId '%s' already exists"), *ShipId.ToString());
		return false;
	}

	// Create new connection entry
	FAndyPortConnection& NewConnection = Connections.Add(ShipId);
	NewConnection.PortName = PortName;
	NewConnection.BaudRate = BaudRate;
	NewConnection.bAutoStart = true;

	// Create the serial port object
	NewConnection.SerialPort = NewObject<UArduinoSerialPort>(this);

	// Create the parser for this connection
	NewConnection.Parser = CreateParserForConnection(ShipId);

	// Create event handler and bind to port
	NewConnection.EventHandler = NewObject<UAndyPortEventHandler>(this);
	NewConnection.EventHandler->Setup(this, ShipId);
	NewConnection.EventHandler->BindToPort(NewConnection.SerialPort);

	UE_LOG(LogTemp, Log, TEXT("AndySerialSubsystem: Added port for ShipId '%s' on %s @ %d baud"),
		*ShipId.ToString(), *PortName, BaudRate);

	return true;
}

bool UAndySerialSubsystem::RemovePort(FName ShipId)
{
	FAndyPortConnection* Connection = Connections.Find(ShipId);
	if (!Connection)
	{
		UE_LOG(LogTemp, Warning, TEXT("AndySerialSubsystem: ShipId '%s' not found"), *ShipId.ToString());
		return false;
	}

	// Close the port if open
	if (Connection->SerialPort && Connection->SerialPort->IsOpen())
	{
		Connection->SerialPort->Close();
	}

	// Unbind event handler
	if (Connection->EventHandler && Connection->SerialPort)
	{
		Connection->EventHandler->UnbindFromPort(Connection->SerialPort);
	}

	// Remove from map
	Connections.Remove(ShipId);

	UE_LOG(LogTemp, Log, TEXT("AndySerialSubsystem: Removed port for ShipId '%s'"), *ShipId.ToString());

	return true;
}

void UAndySerialSubsystem::StartAll()
{
	UE_LOG(LogTemp, Log, TEXT("AndySerialSubsystem: Starting all ports (%d registered)"), Connections.Num());

	for (auto& Pair : Connections)
	{
		FName ShipId = Pair.Key;
		FAndyPortConnection& Connection = Pair.Value;

		if (!Connection.SerialPort)
		{
			UE_LOG(LogTemp, Warning, TEXT("AndySerialSubsystem: No serial port for ShipId '%s'"), *ShipId.ToString());
			continue;
		}

		if (Connection.SerialPort->IsOpen())
		{
			UE_LOG(LogTemp, Log, TEXT("AndySerialSubsystem: ShipId '%s' already open"), *ShipId.ToString());
			continue;
		}

		// Attempt to open the port
		bool bSuccess = Connection.SerialPort->Open(Connection.PortName, Connection.BaudRate);

		if (bSuccess)
		{
			UE_LOG(LogTemp, Log, TEXT("AndySerialSubsystem: Opened port for ShipId '%s' on %s"),
				*ShipId.ToString(), *Connection.PortName);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("AndySerialSubsystem: Failed to open port for ShipId '%s' on %s"),
				*ShipId.ToString(), *Connection.PortName);
		}
	}
}

void UAndySerialSubsystem::StopAll()
{
	UE_LOG(LogTemp, Log, TEXT("AndySerialSubsystem: Stopping all ports"));

	for (auto& Pair : Connections)
	{
		FName ShipId = Pair.Key;
		FAndyPortConnection& Connection = Pair.Value;

		if (Connection.SerialPort && Connection.SerialPort->IsOpen())
		{
			Connection.SerialPort->Close();
			UE_LOG(LogTemp, Log, TEXT("AndySerialSubsystem: Closed port for ShipId '%s'"), *ShipId.ToString());
		}

		// Reset the parser buffer
		if (Connection.Parser)
		{
			Connection.Parser->ResetBuffer();
		}
	}
}

bool UAndySerialSubsystem::IsConnected(FName ShipId) const
{
	const FAndyPortConnection* Connection = Connections.Find(ShipId);
	if (!Connection || !Connection->SerialPort)
	{
		return false;
	}
	return Connection->SerialPort->IsOpen();
}

TArray<FName> UAndySerialSubsystem::GetAllShipIds() const
{
	TArray<FName> ShipIds;
	Connections.GetKeys(ShipIds);
	return ShipIds;
}

bool UAndySerialSubsystem::SendBytes(FName ShipId, const TArray<uint8>& Data)
{
	const FAndyPortConnection* Connection = Connections.Find(ShipId);
	if (!Connection || !Connection->SerialPort || !Connection->SerialPort->IsOpen())
	{
		UE_LOG(LogTemp, Warning, TEXT("AndySerialSubsystem: Cannot send bytes - ShipId '%s' not connected"), *ShipId.ToString());
		return false;
	}

	// Convert bytes to string and send via SendCommand
	FString DataStr;
	for (uint8 Byte : Data)
	{
		DataStr.AppendChar(static_cast<TCHAR>(Byte));
	}
	return Connection->SerialPort->SendCommand(DataStr);
}

bool UAndySerialSubsystem::SendLine(FName ShipId, const FString& Line)
{
	const FAndyPortConnection* Connection = Connections.Find(ShipId);
	if (!Connection || !Connection->SerialPort || !Connection->SerialPort->IsOpen())
	{
		UE_LOG(LogTemp, Warning, TEXT("AndySerialSubsystem: Cannot send line - ShipId '%s' not connected"), *ShipId.ToString());
		return false;
	}

	return Connection->SerialPort->SendLine(Line);
}

void UAndySerialSubsystem::HandleBytesReceived(FName ShipId, const TArray<uint8>& Bytes)
{
	FAndyPortConnection* Connection = Connections.Find(ShipId);
	if (!Connection || !Connection->Parser)
	{
		return;
	}

	// Feed bytes to the parser
	TArray<FBenchPacket> Packets;
	int32 BytesDropped = 0;
	int32 BadEndFrames = 0;
	int32 CrcMismatches = 0;

	int32 PacketCount = Connection->Parser->IngestAndParse(Bytes, Packets, BytesDropped, BadEndFrames, CrcMismatches);

	// Broadcast parsed packets
	for (const FBenchPacket& Packet : Packets)
	{
		HandlePacketDecoded(ShipId, Packet);
	}
}

void UAndySerialSubsystem::HandleConnectionChanged(FName ShipId, bool bConnected)
{
	// Broadcast on game thread
	if (IsInGameThread())
	{
		OnConnectionChanged.Broadcast(ShipId, bConnected);
	}
	else
	{
		// Queue for game thread execution
		AsyncTask(ENamedThreads::GameThread, [this, ShipId, bConnected]()
		{
			if (this && IsValid(this))
			{
				OnConnectionChanged.Broadcast(ShipId, bConnected);
			}
		});
	}
}

void UAndySerialSubsystem::HandlePacketDecoded(FName ShipId, const FBenchPacket& Packet)
{
	// Broadcast on game thread
	if (IsInGameThread())
	{
		OnFrameParsed.Broadcast(ShipId, Packet.Src, Packet.Type, Packet.Seq, Packet.Payload);
	}
	else
	{
		// Copy packet data for async task
		TArray<uint8> PayloadCopy = Packet.Payload;
		uint8 Src = Packet.Src;
		uint8 Type = Packet.Type;
		int32 Seq = Packet.Seq;

		AsyncTask(ENamedThreads::GameThread, [this, ShipId, Src, Type, Seq, PayloadCopy]()
		{
			if (this && IsValid(this))
			{
				OnFrameParsed.Broadcast(ShipId, Src, Type, Seq, PayloadCopy);
			}
		});
	}
}

UByteStreamPacketParser* UAndySerialSubsystem::CreateParserForConnection(FName ShipId)
{
	UByteStreamPacketParser* Parser = NewObject<UByteStreamPacketParser>(this);

	// Configure parser defaults
	Parser->MaxBufferBytes = 4096;
	Parser->MaxPacketsPerCall = 200;
	Parser->bBroadcastPackets = false; // We handle broadcasting ourselves

	return Parser;
}
