// Arduino Communication Plugin - Packet Parser Component Implementation

#include "PacketParserComponent.h"

UPacketParserComponent::UPacketParserComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPacketParserComponent::BeginPlay()
{
	Super::BeginPlay();
	InitializeParser();
}

void UPacketParserComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Unbind events
	if (Parser)
	{
		Parser->OnPacketDecoded.RemoveDynamic(this, &UPacketParserComponent::HandlePacketDecoded);
		Parser->OnBytesDropped.RemoveDynamic(this, &UPacketParserComponent::HandleBytesDropped);
		Parser->OnBadEndFrame.RemoveDynamic(this, &UPacketParserComponent::HandleBadEndFrame);
	}

	Super::EndPlay(EndPlayReason);
}

void UPacketParserComponent::InitializeParser()
{
	// Create parser instance
	Parser = NewObject<UByteStreamPacketParser>(this, UByteStreamPacketParser::StaticClass());

	if (Parser)
	{
		// Apply component configuration
		Parser->MaxBufferBytes = MaxBufferBytes;
		Parser->TrimToBytes = TrimToBytes;
		Parser->MaxPacketsPerCall = MaxPacketsPerCall;
		Parser->bBroadcastPackets = true;
		Parser->bDebugMode = bDebugMode;
		Parser->DebugSampleInterval = DebugSampleInterval;

		// Bind parser events to our handlers
		Parser->OnPacketDecoded.AddDynamic(this, &UPacketParserComponent::HandlePacketDecoded);
		Parser->OnBytesDropped.AddDynamic(this, &UPacketParserComponent::HandleBytesDropped);
		Parser->OnBadEndFrame.AddDynamic(this, &UPacketParserComponent::HandleBadEndFrame);
	}
}

void UPacketParserComponent::IngestBytes(const TArray<uint8>& InBytes)
{
	if (!Parser)
	{
		InitializeParser();
	}

	if (Parser)
	{
		TArray<FBenchPacket> Packets;
		int32 BytesDropped = 0;
		int32 BadEndFrames = 0;

		// IngestAndParse will append, parse, and trigger events
		Parser->IngestAndParse(InBytes, Packets, BytesDropped, BadEndFrames);

		// Note: Events are already fired from within ParsePackets via the parser's broadcasts
	}
}

int32 UPacketParserComponent::FlushAndParse(TArray<FBenchPacket>& OutPackets)
{
	if (!Parser)
	{
		OutPackets.Reset();
		return 0;
	}

	int32 BytesDropped = 0;
	int32 BadEndFrames = 0;
	return Parser->ParsePackets(OutPackets, BytesDropped, BadEndFrames);
}

void UPacketParserComponent::ResetParser()
{
	if (Parser)
	{
		Parser->ResetBuffer();
	}
}

int32 UPacketParserComponent::GetBufferedByteCount() const
{
	return Parser ? Parser->GetBufferedByteCount() : 0;
}

int64 UPacketParserComponent::GetTotalBytesIn() const
{
	return Parser ? Parser->TotalBytesIn : 0;
}

int64 UPacketParserComponent::GetTotalPacketsDecoded() const
{
	return Parser ? Parser->TotalPacketsDecoded : 0;
}

int64 UPacketParserComponent::GetTotalBytesDropped() const
{
	return Parser ? Parser->TotalBytesDropped : 0;
}

int64 UPacketParserComponent::GetTotalBadEndFrames() const
{
	return Parser ? Parser->TotalBadEndFrames : 0;
}

int32 UPacketParserComponent::GetBufferSize() const
{
	return Parser ? Parser->GetBufferSize() : 0;
}

void UPacketParserComponent::ResetStatistics()
{
	if (Parser)
	{
		Parser->ResetStatistics();
	}
}

void UPacketParserComponent::HandlePacketDecoded(FBenchPacket Packet)
{
	// Forward to component's delegate
	OnPacketDecoded.Broadcast(Packet);
}

void UPacketParserComponent::HandleBytesDropped(int32 ByteCount)
{
	// Forward to component's delegate
	OnBytesDropped.Broadcast(ByteCount);
}

void UPacketParserComponent::HandleBadEndFrame()
{
	// Forward to component's delegate
	OnBadEndFrame.Broadcast();
}
