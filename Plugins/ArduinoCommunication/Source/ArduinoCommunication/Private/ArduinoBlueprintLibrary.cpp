// Arduino Communication Plugin - Blueprint Function Library Implementation

#include "ArduinoBlueprintLibrary.h"
#include "Interfaces/IPv4/IPv4Address.h"

TArray<FString> UArduinoBlueprintLibrary::GetAvailableComPorts()
{
	return UArduinoSerialPort::GetAvailablePorts();
}

void UArduinoBlueprintLibrary::ParseArduinoResponse(const FString& Response, FString& OutType, FString& OutData)
{
	int32 ColonIndex;
	if (Response.FindChar(':', ColonIndex))
	{
		OutType = Response.Left(ColonIndex);
		OutData = Response.Mid(ColonIndex + 1);
	}
	else
	{
		OutType = Response;
		OutData = TEXT("");
	}
}

FString UArduinoBlueprintLibrary::MakeCommand(const FString& Command, const FString& Parameter)
{
	if (Parameter.IsEmpty())
	{
		return Command;
	}
	return FString::Printf(TEXT("%s:%s"), *Command, *Parameter);
}

bool UArduinoBlueprintLibrary::IsValidIPAddress(const FString& IPAddress)
{
	FIPv4Address IP;
	return FIPv4Address::Parse(IPAddress, IP);
}

int32 UArduinoBlueprintLibrary::ParseIntFromResponse(const FString& Data, int32 DefaultValue)
{
	if (Data.IsNumeric())
	{
		return FCString::Atoi(*Data);
	}
	return DefaultValue;
}

FString UArduinoBlueprintLibrary::FloatToArduinoString(float Value, int32 DecimalPlaces)
{
	return FString::Printf(TEXT("%.*f"), DecimalPlaces, Value);
}

bool UArduinoBlueprintLibrary::ParseKeyValue(const FString& Data, const FString& Key, FString& OutValue)
{
	FString SearchKey = Key + TEXT("=");
	int32 KeyIndex = Data.Find(SearchKey, ESearchCase::IgnoreCase);

	if (KeyIndex == INDEX_NONE)
	{
		OutValue = TEXT("");
		return false;
	}

	int32 ValueStart = KeyIndex + SearchKey.Len();
	int32 ValueEnd = Data.Find(TEXT(","), ESearchCase::IgnoreCase, ESearchDir::FromStart, ValueStart);

	if (ValueEnd == INDEX_NONE)
	{
		OutValue = Data.Mid(ValueStart);
	}
	else
	{
		OutValue = Data.Mid(ValueStart, ValueEnd - ValueStart);
	}

	return true;
}

UArduinoSerialPort* UArduinoBlueprintLibrary::CreateSerialPort(UObject* WorldContextObject)
{
	if (WorldContextObject == nullptr)
	{
		return nullptr;
	}

	return NewObject<UArduinoSerialPort>(WorldContextObject);
}

UArduinoTcpClient* UArduinoBlueprintLibrary::CreateTcpClient(UObject* WorldContextObject)
{
	if (WorldContextObject == nullptr)
	{
		return nullptr;
	}

	return NewObject<UArduinoTcpClient>(WorldContextObject);
}

UByteStreamPacketParser* UArduinoBlueprintLibrary::CreateByteStreamPacketParser(UObject* WorldContextObject)
{
	if (WorldContextObject == nullptr)
	{
		return nullptr;
	}

	return NewObject<UByteStreamPacketParser>(WorldContextObject);
}

TArray<uint8> UArduinoBlueprintLibrary::CreateTestByteStream()
{
	TArray<uint8> TestBytes;

	// Junk prefix (3 bytes of garbage)
	TestBytes.Add(0x12);
	TestBytes.Add(0x34);
	TestBytes.Add(0x56);

	// Helper lambda to compute CRC (XOR of bytes 1..6+Len)
	auto ComputeCrc = [](const TArray<uint8>& Bytes, int32 Start, int32 Len) -> uint8 {
		uint8 Crc = 0;
		for (int32 i = Start + 1; i <= Start + 6 + Len; i++)
		{
			Crc ^= Bytes[i];
		}
		return Crc;
	};

	// Valid packet 1: Ver=1, Src=2, Type=3, Seq=0x0102 (258), Len=2, Payload=[0xAB, 0xCD]
	// Frame: [0xAA][VER][SRC][TYPE][SEQ_L][SEQ_H][LEN][PAYLOAD...][CRC][0x55]
	int32 Pkt1Start = TestBytes.Num();
	TestBytes.Add(0xAA); // START
	TestBytes.Add(0x01); // VER
	TestBytes.Add(0x02); // SRC
	TestBytes.Add(0x03); // TYPE
	TestBytes.Add(0x02); // SEQ_L
	TestBytes.Add(0x01); // SEQ_H
	TestBytes.Add(0x02); // LEN (2 bytes payload)
	TestBytes.Add(0xAB); // Payload[0]
	TestBytes.Add(0xCD); // Payload[1]
	TestBytes.Add(ComputeCrc(TestBytes, Pkt1Start, 2)); // CRC
	TestBytes.Add(0x55); // END

	// Valid packet 2: Ver=1, Src=0, Type=5, Seq=0x0001 (1), Len=0 (no payload)
	int32 Pkt2Start = TestBytes.Num();
	TestBytes.Add(0xAA); // START
	TestBytes.Add(0x01); // VER
	TestBytes.Add(0x00); // SRC
	TestBytes.Add(0x05); // TYPE
	TestBytes.Add(0x01); // SEQ_L
	TestBytes.Add(0x00); // SEQ_H
	TestBytes.Add(0x00); // LEN (0 bytes payload)
	TestBytes.Add(ComputeCrc(TestBytes, Pkt2Start, 0)); // CRC
	TestBytes.Add(0x55); // END

	// Bad packet (wrong end byte - should trigger resync)
	int32 BadPktStart = TestBytes.Num();
	TestBytes.Add(0xAA); // START
	TestBytes.Add(0x01); // VER
	TestBytes.Add(0x99); // SRC
	TestBytes.Add(0x88); // TYPE
	TestBytes.Add(0x77); // SEQ_L
	TestBytes.Add(0x66); // SEQ_H
	TestBytes.Add(0x00); // LEN (0 bytes)
	TestBytes.Add(ComputeCrc(TestBytes, BadPktStart, 0)); // CRC (correct, but end is wrong)
	TestBytes.Add(0x00); // BAD END (should be 0x55)

	// Valid packet 3 after bad frame: Ver=2, Src=0, Type=1, Seq=0x1234 (4660), Len=1, Payload=[0xFF]
	int32 Pkt3Start = TestBytes.Num();
	TestBytes.Add(0xAA); // START
	TestBytes.Add(0x02); // VER
	TestBytes.Add(0x00); // SRC
	TestBytes.Add(0x01); // TYPE
	TestBytes.Add(0x34); // SEQ_L
	TestBytes.Add(0x12); // SEQ_H
	TestBytes.Add(0x01); // LEN (1 byte payload)
	TestBytes.Add(0xFF); // Payload[0]
	TestBytes.Add(ComputeCrc(TestBytes, Pkt3Start, 1)); // CRC
	TestBytes.Add(0x55); // END

	// Partial packet at end (only header, waiting for more)
	TestBytes.Add(0xAA); // START
	TestBytes.Add(0x01); // VER
	TestBytes.Add(0xDE); // SRC
	TestBytes.Add(0xAD); // TYPE (incomplete - missing SEQ, LEN, CRC, END)

	return TestBytes;
}

bool UArduinoBlueprintLibrary::RunPacketParserTest(UObject* WorldContextObject, FString& OutTestLog)
{
	OutTestLog.Empty();
	bool bAllPassed = true;

	// Create parser
	UByteStreamPacketParser* Parser = CreateByteStreamPacketParser(WorldContextObject);
	if (!Parser)
	{
		OutTestLog = TEXT("FAIL: Could not create parser");
		return false;
	}

	// Helper lambda to compute CRC for test frames
	auto ComputeCrc = [](const TArray<uint8>& Bytes, int32 Start, int32 Len) -> uint8 {
		uint8 Crc = 0;
		for (int32 i = Start + 1; i <= Start + 6 + Len; i++)
		{
			Crc ^= Bytes[i];
		}
		return Crc;
	};

	OutTestLog += TEXT("=== Packet Parser Test Suite (Framed Protocol) ===\n\n");

	// Test 1: Parse complete test stream in one chunk
	{
		OutTestLog += TEXT("Test 1: Full stream parsing\n");
		Parser->ResetBuffer();
		Parser->ResetStatistics();

		TArray<uint8> TestBytes = CreateTestByteStream();
		TArray<FBenchPacket> Packets;
		int32 BytesDropped = 0;
		int32 BadEndFrames = 0;
		int32 CrcMismatches = 0;

		Parser->IngestAndParse(TestBytes, Packets, BytesDropped, BadEndFrames, CrcMismatches);

		// Expected: 3 valid packets, 3 bytes junk + 1 bad start = 4 dropped, 1 bad end frame
		bool bPass = (Packets.Num() == 3);
		bPass = bPass && (BadEndFrames == 1);
		bPass = bPass && (Parser->GetBufferedByteCount() == 4); // Partial packet remains (4 bytes: START, VER, SRC, TYPE)

		if (bPass && Packets.Num() >= 3)
		{
			// Verify packet 1 contents: Ver=1, Src=2, Type=3, Seq=258, Len=2, Payload=[0xAB, 0xCD]
			bPass = bPass && (Packets[0].Ver == 1 && Packets[0].Src == 2 && Packets[0].Type == 3);
			bPass = bPass && (Packets[0].Seq == 258 && Packets[0].Len == 2);
			bPass = bPass && (Packets[0].Payload.Num() == 2 && Packets[0].Payload[0] == 0xAB && Packets[0].Payload[1] == 0xCD);

			// Verify packet 2 contents: Ver=1, Src=0, Type=5, Seq=1, Len=0
			bPass = bPass && (Packets[1].Ver == 1 && Packets[1].Src == 0 && Packets[1].Type == 5);
			bPass = bPass && (Packets[1].Seq == 1 && Packets[1].Len == 0);
			bPass = bPass && (Packets[1].Payload.Num() == 0);

			// Verify packet 3 contents: Ver=2, Src=0, Type=1, Seq=4660, Len=1, Payload=[0xFF]
			bPass = bPass && (Packets[2].Ver == 2 && Packets[2].Src == 0 && Packets[2].Type == 1);
			bPass = bPass && (Packets[2].Seq == 4660 && Packets[2].Len == 1);
			bPass = bPass && (Packets[2].Payload.Num() == 1 && Packets[2].Payload[0] == 0xFF);
		}

		OutTestLog += FString::Printf(TEXT("  Packets decoded: %d (expected 3)\n"), Packets.Num());
		OutTestLog += FString::Printf(TEXT("  Bytes dropped: %d\n"), BytesDropped);
		OutTestLog += FString::Printf(TEXT("  Bad end frames: %d (expected 1)\n"), BadEndFrames);
		OutTestLog += FString::Printf(TEXT("  CRC mismatches: %d (expected 0)\n"), CrcMismatches);
		OutTestLog += FString::Printf(TEXT("  Buffered bytes: %d (expected 4)\n"), Parser->GetBufferedByteCount());
		OutTestLog += FString::Printf(TEXT("  Result: %s\n\n"), bPass ? TEXT("PASS") : TEXT("FAIL"));
		bAllPassed = bAllPassed && bPass;
	}

	// Test 2: Split packet across multiple chunks
	{
		OutTestLog += TEXT("Test 2: Split packet handling\n");
		Parser->ResetBuffer();
		Parser->ResetStatistics();

		// First chunk: partial frame (START + partial header)
		TArray<uint8> Chunk1;
		Chunk1.Add(0xAA); // START
		Chunk1.Add(0x01); // VER
		Chunk1.Add(0x10); // SRC
		Chunk1.Add(0x20); // TYPE

		TArray<FBenchPacket> Packets1;
		int32 Dropped1 = 0, BadEnd1 = 0, CrcMismatch1 = 0;
		Parser->IngestAndParse(Chunk1, Packets1, Dropped1, BadEnd1, CrcMismatch1);

		bool bPass = (Packets1.Num() == 0); // Should have no packets yet
		bPass = bPass && (Parser->GetBufferedByteCount() == 4);

		// Second chunk: rest of frame (SEQ_L, SEQ_H, LEN=0, CRC, END)
		TArray<uint8> Chunk2;
		Chunk2.Add(0x30); // SEQ_L
		Chunk2.Add(0x40); // SEQ_H
		Chunk2.Add(0x00); // LEN=0

		// Compute CRC: We need to calculate it for the complete frame
		// Frame is: [0xAA][0x01][0x10][0x20][0x30][0x40][0x00]
		// CRC = XOR of bytes 1..6 = 0x01 ^ 0x10 ^ 0x20 ^ 0x30 ^ 0x40 ^ 0x00 = 0x61
		uint8 ExpectedCrc = 0x01 ^ 0x10 ^ 0x20 ^ 0x30 ^ 0x40 ^ 0x00;
		Chunk2.Add(ExpectedCrc); // CRC
		Chunk2.Add(0x55); // END

		TArray<FBenchPacket> Packets2;
		int32 Dropped2 = 0, BadEnd2 = 0, CrcMismatch2 = 0;
		Parser->IngestAndParse(Chunk2, Packets2, Dropped2, BadEnd2, CrcMismatch2);

		bPass = bPass && (Packets2.Num() == 1);
		if (Packets2.Num() > 0)
		{
			bPass = bPass && (Packets2[0].Ver == 0x01);
			bPass = bPass && (Packets2[0].Src == 0x10);
			bPass = bPass && (Packets2[0].Type == 0x20);
			bPass = bPass && (Packets2[0].Seq == 0x4030);
			bPass = bPass && (Packets2[0].Len == 0);
		}

		OutTestLog += FString::Printf(TEXT("  Chunk 1 packets: %d (expected 0)\n"), Packets1.Num());
		OutTestLog += FString::Printf(TEXT("  Chunk 2 packets: %d (expected 1)\n"), Packets2.Num());
		if (Packets2.Num() > 0)
		{
			OutTestLog += FString::Printf(TEXT("  Decoded Seq: 0x%04X (expected 0x4030)\n"), Packets2[0].Seq);
		}
		OutTestLog += FString::Printf(TEXT("  Result: %s\n\n"), bPass ? TEXT("PASS") : TEXT("FAIL"));
		bAllPassed = bAllPassed && bPass;
	}

	// Test 3: All junk bytes (no valid start)
	{
		OutTestLog += TEXT("Test 3: All junk handling\n");
		Parser->ResetBuffer();
		Parser->ResetStatistics();

		TArray<uint8> Junk;
		for (int32 i = 0; i < 20; i++)
		{
			Junk.Add(static_cast<uint8>(i)); // No 0xAA in this range
		}

		TArray<FBenchPacket> Packets;
		int32 Dropped = 0, BadEnd = 0, CrcMismatch = 0;
		Parser->IngestAndParse(Junk, Packets, Dropped, BadEnd, CrcMismatch);

		bool bPass = (Packets.Num() == 0);
		bPass = bPass && (Dropped == 20);
		bPass = bPass && (Parser->GetBufferedByteCount() == 0);

		OutTestLog += FString::Printf(TEXT("  Packets: %d (expected 0)\n"), Packets.Num());
		OutTestLog += FString::Printf(TEXT("  Bytes dropped: %d (expected 20)\n"), Dropped);
		OutTestLog += FString::Printf(TEXT("  Result: %s\n\n"), bPass ? TEXT("PASS") : TEXT("FAIL"));
		bAllPassed = bAllPassed && bPass;
	}

	// Test 4: CRC mismatch detection
	{
		OutTestLog += TEXT("Test 4: CRC mismatch detection\n");
		Parser->ResetBuffer();
		Parser->ResetStatistics();

		TArray<uint8> BadCrcFrame;
		// Frame with bad CRC: Ver=1, Src=0, Type=1, Seq=1, Len=0
		BadCrcFrame.Add(0xAA); // START
		BadCrcFrame.Add(0x01); // VER
		BadCrcFrame.Add(0x00); // SRC
		BadCrcFrame.Add(0x01); // TYPE
		BadCrcFrame.Add(0x01); // SEQ_L
		BadCrcFrame.Add(0x00); // SEQ_H
		BadCrcFrame.Add(0x00); // LEN=0
		BadCrcFrame.Add(0xFF); // BAD CRC (correct would be 0x01 ^ 0x00 ^ 0x01 ^ 0x01 ^ 0x00 ^ 0x00 = 0x01)
		BadCrcFrame.Add(0x55); // END

		// Valid frame after bad CRC frame
		int32 ValidStart = BadCrcFrame.Num();
		BadCrcFrame.Add(0xAA); // START
		BadCrcFrame.Add(0x01); // VER
		BadCrcFrame.Add(0x02); // SRC
		BadCrcFrame.Add(0x03); // TYPE
		BadCrcFrame.Add(0x04); // SEQ_L
		BadCrcFrame.Add(0x05); // SEQ_H
		BadCrcFrame.Add(0x00); // LEN=0
		BadCrcFrame.Add(ComputeCrc(BadCrcFrame, ValidStart, 0)); // Correct CRC
		BadCrcFrame.Add(0x55); // END

		TArray<FBenchPacket> Packets;
		int32 Dropped = 0, BadEnd = 0, CrcMismatch = 0;
		Parser->IngestAndParse(BadCrcFrame, Packets, Dropped, BadEnd, CrcMismatch);

		bool bPass = (Packets.Num() == 1);
		bPass = bPass && (CrcMismatch == 1);
		if (Packets.Num() > 0)
		{
			bPass = bPass && (Packets[0].Ver == 1 && Packets[0].Src == 2 && Packets[0].Type == 3);
			bPass = bPass && (Packets[0].Seq == 0x0504);
		}

		OutTestLog += FString::Printf(TEXT("  Packets: %d (expected 1)\n"), Packets.Num());
		OutTestLog += FString::Printf(TEXT("  CRC mismatches: %d (expected 1)\n"), CrcMismatch);
		OutTestLog += FString::Printf(TEXT("  Result: %s\n\n"), bPass ? TEXT("PASS") : TEXT("FAIL"));
		bAllPassed = bAllPassed && bPass;
	}

	// Test 5: Buffer overflow protection
	{
		OutTestLog += TEXT("Test 5: Buffer overflow protection\n");
		Parser->ResetBuffer();
		Parser->ResetStatistics();
		Parser->MaxBufferBytes = 100;
		Parser->TrimToBytes = 10;

		// Add more than MaxBufferBytes of junk without any start byte
		TArray<uint8> BigJunk;
		for (int32 i = 0; i < 200; i++)
		{
			BigJunk.Add(0x01); // No 0xAA
		}

		TArray<FBenchPacket> Packets;
		int32 Dropped = 0, BadEnd = 0, CrcMismatch = 0;
		Parser->IngestAndParse(BigJunk, Packets, Dropped, BadEnd, CrcMismatch);

		// Buffer should have been trimmed
		bool bPass = (Parser->GetBufferedByteCount() <= Parser->TrimToBytes || Parser->GetBufferedByteCount() == 0);

		OutTestLog += FString::Printf(TEXT("  Buffered after overflow: %d (expected <= %d)\n"),
			Parser->GetBufferedByteCount(), Parser->TrimToBytes);
		OutTestLog += FString::Printf(TEXT("  Result: %s\n\n"), bPass ? TEXT("PASS") : TEXT("FAIL"));
		bAllPassed = bAllPassed && bPass;
	}

	// Test 6: Variable payload length handling
	{
		OutTestLog += TEXT("Test 6: Variable payload length\n");
		Parser->ResetBuffer();
		Parser->ResetStatistics();
		Parser->MaxBufferBytes = 4096;
		Parser->TrimToBytes = 64;

		TArray<uint8> VarPayloadFrames;

		// Packet with 5 byte payload
		int32 Start1 = VarPayloadFrames.Num();
		VarPayloadFrames.Add(0xAA);
		VarPayloadFrames.Add(0x01); // VER
		VarPayloadFrames.Add(0x00); // SRC
		VarPayloadFrames.Add(0x01); // TYPE
		VarPayloadFrames.Add(0x01); // SEQ_L
		VarPayloadFrames.Add(0x00); // SEQ_H
		VarPayloadFrames.Add(0x05); // LEN=5
		VarPayloadFrames.Add(0x11); // Payload[0]
		VarPayloadFrames.Add(0x22); // Payload[1]
		VarPayloadFrames.Add(0x33); // Payload[2]
		VarPayloadFrames.Add(0x44); // Payload[3]
		VarPayloadFrames.Add(0x55); // Payload[4] (note: same as END byte, but in payload)
		VarPayloadFrames.Add(ComputeCrc(VarPayloadFrames, Start1, 5)); // CRC
		VarPayloadFrames.Add(0x55); // END

		TArray<FBenchPacket> Packets;
		int32 Dropped = 0, BadEnd = 0, CrcMismatch = 0;
		Parser->IngestAndParse(VarPayloadFrames, Packets, Dropped, BadEnd, CrcMismatch);

		bool bPass = (Packets.Num() == 1);
		if (Packets.Num() > 0)
		{
			bPass = bPass && (Packets[0].Len == 5);
			bPass = bPass && (Packets[0].Payload.Num() == 5);
			bPass = bPass && (Packets[0].Payload[0] == 0x11);
			bPass = bPass && (Packets[0].Payload[4] == 0x55);
		}

		OutTestLog += FString::Printf(TEXT("  Packets: %d (expected 1)\n"), Packets.Num());
		if (Packets.Num() > 0)
		{
			OutTestLog += FString::Printf(TEXT("  Payload length: %d (expected 5)\n"), Packets[0].Payload.Num());
		}
		OutTestLog += FString::Printf(TEXT("  Result: %s\n\n"), bPass ? TEXT("PASS") : TEXT("FAIL"));
		bAllPassed = bAllPassed && bPass;
	}

	OutTestLog += FString::Printf(TEXT("=== Overall Result: %s ===\n"), bAllPassed ? TEXT("ALL TESTS PASSED") : TEXT("SOME TESTS FAILED"));

	return bAllPassed;
}
