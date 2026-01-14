// Arduino Communication Plugin - Binary Packet Parser Implementation

#include "ByteStreamPacketParser.h"

UByteStreamPacketParser::UByteStreamPacketParser()
{
	// Pre-allocate some buffer capacity to reduce reallocations
	Buffer.Reserve(1024);
}

void UByteStreamPacketParser::AppendBytes(const TArray<uint8>& InBytes)
{
	if (InBytes.Num() == 0)
	{
		return;
	}

	Buffer.Append(InBytes);
	EnforceBufferLimits();
}

int32 UByteStreamPacketParser::ParsePackets(TArray<FBenchPacket>& OutPackets, int32& OutBytesDropped, int32& OutBadEndFrames)
{
	OutPackets.Reset();
	OutBytesDropped = 0;
	OutBadEndFrames = 0;

	int32 PacketsParsed = 0;
	int32 ReadIndex = 0;

	// Parse up to MaxPacketsPerCall to prevent infinite loops
	while (PacketsParsed < MaxPacketsPerCall)
	{
		// Find the first start byte from current read position
		int32 StartIndex = FindStartByte(ReadIndex);

		if (StartIndex == INDEX_NONE)
		{
			// No start byte found - all remaining bytes are junk
			int32 BytesToDrop = Buffer.Num() - ReadIndex;
			if (BytesToDrop > 0)
			{
				OutBytesDropped += BytesToDrop;
				TotalBytesDropped += BytesToDrop;
				if (OnBytesDropped.IsBound())
				{
					OnBytesDropped.Broadcast(BytesToDrop);
				}
			}
			// Clear the entire buffer
			Buffer.Reset();
			break;
		}

		// Discard any bytes before the start marker (junk)
		if (StartIndex > ReadIndex)
		{
			int32 JunkBytes = StartIndex - ReadIndex;
			OutBytesDropped += JunkBytes;
			TotalBytesDropped += JunkBytes;
			if (OnBytesDropped.IsBound())
			{
				OnBytesDropped.Broadcast(JunkBytes);
			}
		}

		ReadIndex = StartIndex;

		// Check if we have enough bytes for a complete packet
		int32 BytesRemaining = Buffer.Num() - ReadIndex;
		if (BytesRemaining < PacketSize)
		{
			// Not enough data yet - wait for more
			break;
		}

		// Validate end byte
		if (Buffer[ReadIndex + 5] != EndByte)
		{
			// Bad end frame - discard only the start byte and resync
			OutBadEndFrames++;
			TotalBadEndFrames++;
			OutBytesDropped++;
			TotalBytesDropped++;
			if (OnBadEndFrame.IsBound())
			{
				OnBadEndFrame.Broadcast();
			}
			if (OnBytesDropped.IsBound())
			{
				OnBytesDropped.Broadcast(1);
			}
			ReadIndex++;
			continue;
		}

		// Valid packet - decode it
		FBenchPacket Packet = DecodePacketAt(ReadIndex);
		OutPackets.Add(Packet);
		PacketsParsed++;
		TotalPacketsDecoded++;

		// Broadcast event if enabled
		if (bBroadcastPackets && OnPacketDecoded.IsBound())
		{
			OnPacketDecoded.Broadcast(Packet);
		}

		// Consume the packet bytes
		ReadIndex += PacketSize;
	}

	// Compact the buffer by removing consumed bytes
	if (ReadIndex > 0)
	{
		if (ReadIndex >= Buffer.Num())
		{
			Buffer.Reset();
		}
		else
		{
			// Shift remaining bytes to front
			int32 RemainingBytes = Buffer.Num() - ReadIndex;
			FMemory::Memmove(Buffer.GetData(), Buffer.GetData() + ReadIndex, RemainingBytes);
			Buffer.SetNum(RemainingBytes, false);
		}
	}

	return PacketsParsed;
}

int32 UByteStreamPacketParser::IngestAndParse(const TArray<uint8>& InBytes, TArray<FBenchPacket>& OutPackets, int32& OutBytesDropped, int32& OutBadEndFrames)
{
	AppendBytes(InBytes);
	return ParsePackets(OutPackets, OutBytesDropped, OutBadEndFrames);
}

void UByteStreamPacketParser::ResetBuffer()
{
	Buffer.Reset();
}

int32 UByteStreamPacketParser::GetBufferedByteCount() const
{
	return Buffer.Num();
}

void UByteStreamPacketParser::ResetStatistics()
{
	TotalPacketsDecoded = 0;
	TotalBytesDropped = 0;
	TotalBadEndFrames = 0;
}

int32 UByteStreamPacketParser::FindStartByte(int32 StartIndex) const
{
	for (int32 i = StartIndex; i < Buffer.Num(); i++)
	{
		if (Buffer[i] == StartByte)
		{
			return i;
		}
	}
	return INDEX_NONE;
}

FBenchPacket UByteStreamPacketParser::DecodePacketAt(int32 Offset) const
{
	// Packet layout: [0xAA][cycleLo][cycleHi][line][data][0x55]
	uint8 CycleLo = Buffer[Offset + 1];
	uint8 CycleHi = Buffer[Offset + 2];
	uint8 Line = Buffer[Offset + 3];
	uint8 Data = Buffer[Offset + 4];

	// Decode little-endian uint16 cycle
	int32 Cycle = static_cast<int32>(CycleLo) | (static_cast<int32>(CycleHi) << 8);

	return FBenchPacket(Cycle, Line, Data);
}

int32 UByteStreamPacketParser::EnforceBufferLimits()
{
	if (Buffer.Num() <= MaxBufferBytes)
	{
		return 0;
	}

	int32 BytesToTrim;
	if (TrimToBytes > 0 && TrimToBytes < Buffer.Num())
	{
		// Keep last TrimToBytes bytes
		BytesToTrim = Buffer.Num() - TrimToBytes;
		int32 RemainingBytes = TrimToBytes;
		FMemory::Memmove(Buffer.GetData(), Buffer.GetData() + BytesToTrim, RemainingBytes);
		Buffer.SetNum(RemainingBytes, false);
	}
	else
	{
		// Clear entire buffer
		BytesToTrim = Buffer.Num();
		Buffer.Reset();
	}

	TotalBytesDropped += BytesToTrim;
	if (OnBytesDropped.IsBound())
	{
		OnBytesDropped.Broadcast(BytesToTrim);
	}

	return BytesToTrim;
}
