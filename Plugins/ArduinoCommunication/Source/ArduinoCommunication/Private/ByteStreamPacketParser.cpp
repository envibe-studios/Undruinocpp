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

	TotalBytesIn += InBytes.Num();
	Buffer.Append(InBytes);
	EnforceBufferLimits();
}

int32 UByteStreamPacketParser::ParsePackets(TArray<FBenchPacket>& OutPackets, int32& OutBytesDropped, int32& OutBadEndFrames, int32& OutCrcMismatches)
{
	OutPackets.Reset();
	OutBytesDropped = 0;
	OutBadEndFrames = 0;
	OutCrcMismatches = 0;

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

		// Check if we have enough bytes to read the header (START + 6 header bytes)
		int32 BytesRemaining = Buffer.Num() - ReadIndex;
		if (BytesRemaining < MinBytesToReadHeader)
		{
			// Not enough data to read header - wait for more
			break;
		}

		// Read payload length from header (byte 6 after start, i.e., index ReadIndex+6)
		const uint8 PayloadLen = Buffer[ReadIndex + 6];

		// Validate payload length
		if (PayloadLen > MaxPayloadLen)
		{
			// Invalid payload length - discard start byte and resync
			OutBytesDropped++;
			TotalBytesDropped++;
			if (OnBytesDropped.IsBound())
			{
				OnBytesDropped.Broadcast(1);
			}
			ReadIndex++;
			continue;
		}

		// Compute total frame length: 1(start) + 6(header) + LEN(payload) + 1(CRC) + 1(end) = 9 + LEN
		const int32 FrameSize = MinFrameSize + PayloadLen;

		// Check if we have enough bytes for the complete frame
		if (BytesRemaining < FrameSize)
		{
			// Not enough data yet - wait for more
			break;
		}

		// Validate end byte at index start + 8 + LEN (i.e., ReadIndex + 8 + PayloadLen)
		const int32 EndByteIndex = ReadIndex + 8 + PayloadLen;
		if (Buffer[EndByteIndex] != EndByte)
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

		// Validate CRC - XOR of bytes 1..(6+LEN), i.e., bytes at indices ReadIndex+1 to ReadIndex+6+PayloadLen
		const uint8 ExpectedCrc = ComputeCrc(ReadIndex, PayloadLen);
		const uint8 ActualCrc = Buffer[ReadIndex + 7 + PayloadLen]; // CRC is at index start + 7 + LEN

		if (ExpectedCrc != ActualCrc)
		{
			// CRC mismatch - discard start byte and resync
			OutCrcMismatches++;
			TotalCrcMismatches++;
			OutBytesDropped++;
			TotalBytesDropped++;
			if (OnCrcMismatch.IsBound())
			{
				OnCrcMismatch.Broadcast(ExpectedCrc, ActualCrc);
			}
			if (OnBytesDropped.IsBound())
			{
				OnBytesDropped.Broadcast(1);
			}
			ReadIndex++;
			continue;
		}

		// Valid packet - decode it
		FBenchPacket Packet = DecodePacketAt(ReadIndex, PayloadLen);
		OutPackets.Add(Packet);
		PacketsParsed++;
		TotalPacketsDecoded++;

		// Broadcast event if enabled
		if (bBroadcastPackets && OnPacketDecoded.IsBound())
		{
			OnPacketDecoded.Broadcast(Packet);
		}

		// Consume the frame bytes
		ReadIndex += FrameSize;
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

int32 UByteStreamPacketParser::IngestAndParse(const TArray<uint8>& InBytes, TArray<FBenchPacket>& OutPackets, int32& OutBytesDropped, int32& OutBadEndFrames, int32& OutCrcMismatches)
{
	AppendBytes(InBytes);
	return ParsePackets(OutPackets, OutBytesDropped, OutBadEndFrames, OutCrcMismatches);
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
	TotalBytesIn = 0;
	TotalPacketsDecoded = 0;
	TotalBytesDropped = 0;
	TotalBadEndFrames = 0;
	TotalCrcMismatches = 0;
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

FBenchPacket UByteStreamPacketParser::DecodePacketAt(int32 Offset, int32 PayloadLen)
{
	// Frame layout: [0xAA][VER][SRC][TYPE][SEQ_L][SEQ_H][LEN][PAYLOAD...][CRC][0x55]
	//               Offset+0  +1   +2    +3    +4     +5    +6  +7...
	const uint8 Ver = Buffer[Offset + 1];
	const uint8 Src = Buffer[Offset + 2];
	const uint8 Type = Buffer[Offset + 3];
	const uint8 SeqLo = Buffer[Offset + 4];
	const uint8 SeqHi = Buffer[Offset + 5];
	const uint8 Len = Buffer[Offset + 6];

	// Decode little-endian uint16 sequence number
	const int32 Seq = static_cast<int32>(SeqLo) | (static_cast<int32>(SeqHi) << 8);

	// Extract payload bytes
	TArray<uint8> Payload;
	if (PayloadLen > 0)
	{
		Payload.SetNumUninitialized(PayloadLen);
		FMemory::Memcpy(Payload.GetData(), Buffer.GetData() + Offset + 7, PayloadLen);
	}

	// Debug: Log sample packet every DebugSampleInterval packets when debug mode is enabled
	if (bDebugMode && DebugSampleInterval > 0 && (TotalPacketsDecoded + 1) % DebugSampleInterval == 0)
	{
		FString PayloadHex;
		for (int32 i = 0; i < PayloadLen && i < 8; i++)
		{
			PayloadHex += FString::Printf(TEXT("%02X "), Payload[i]);
		}
		if (PayloadLen > 8)
		{
			PayloadHex += TEXT("...");
		}

		UE_LOG(LogTemp, Warning, TEXT("PacketParser Debug [%lld]: Ver=%d Src=%d Type=%d Seq=%d Len=%d Payload=[%s]"),
			TotalPacketsDecoded + 1,
			Ver, Src, Type, Seq, Len, *PayloadHex.TrimEnd());
	}

	return FBenchPacket(Ver, Src, Type, Seq, Len, Payload);
}

uint8 UByteStreamPacketParser::ComputeCrc(int32 Offset, int32 PayloadLen) const
{
	// CRC = XOR of bytes [VER..(LEN+payload)], i.e., bytes at indices Offset+1 to Offset+6+PayloadLen
	// That's bytes 1 through 6+LEN (inclusive), where byte 1 is VER, byte 6 is LEN, bytes 7..6+LEN are payload
	uint8 Crc = 0;
	const int32 CrcEndIndex = Offset + 6 + PayloadLen; // Last byte to include in CRC (last payload byte or LEN if no payload)
	for (int32 i = Offset + 1; i <= CrcEndIndex; i++)
	{
		Crc ^= Buffer[i];
	}
	return Crc;
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
