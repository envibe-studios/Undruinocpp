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

	// Valid packet 1: Cycle=0x0102 (258), Line=0x03, Data=0x04
	TestBytes.Add(0xAA); // START
	TestBytes.Add(0x02); // cycleLo
	TestBytes.Add(0x01); // cycleHi
	TestBytes.Add(0x03); // line
	TestBytes.Add(0x04); // data
	TestBytes.Add(0x55); // END

	// Valid packet 2: Cycle=0x0001 (1), Line=0xFF, Data=0xAB
	TestBytes.Add(0xAA); // START
	TestBytes.Add(0x01); // cycleLo
	TestBytes.Add(0x00); // cycleHi
	TestBytes.Add(0xFF); // line
	TestBytes.Add(0xAB); // data
	TestBytes.Add(0x55); // END

	// Bad packet (wrong end byte - should trigger resync)
	TestBytes.Add(0xAA); // START
	TestBytes.Add(0x99); // cycleLo
	TestBytes.Add(0x88); // cycleHi
	TestBytes.Add(0x77); // line
	TestBytes.Add(0x66); // data
	TestBytes.Add(0x00); // BAD END (should be 0x55)

	// Valid packet 3 after bad frame: Cycle=0x1234 (4660), Line=0x05, Data=0x06
	TestBytes.Add(0xAA); // START
	TestBytes.Add(0x34); // cycleLo
	TestBytes.Add(0x12); // cycleHi
	TestBytes.Add(0x05); // line
	TestBytes.Add(0x06); // data
	TestBytes.Add(0x55); // END

	// Partial packet at end (only 3 bytes, waiting for more)
	TestBytes.Add(0xAA); // START
	TestBytes.Add(0xDE); // cycleLo
	TestBytes.Add(0xAD); // cycleHi (incomplete - no line/data/end)

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

	OutTestLog += TEXT("=== Packet Parser Test Suite ===\n\n");

	// Test 1: Parse complete test stream in one chunk
	{
		OutTestLog += TEXT("Test 1: Full stream parsing\n");
		Parser->ResetBuffer();
		Parser->ResetStatistics();

		TArray<uint8> TestBytes = CreateTestByteStream();
		TArray<FBenchPacket> Packets;
		int32 BytesDropped = 0;
		int32 BadEndFrames = 0;

		Parser->IngestAndParse(TestBytes, Packets, BytesDropped, BadEndFrames);

		// Expected: 3 valid packets, 3 bytes junk + 1 bad start = 4 dropped, 1 bad end frame
		// Also 1 byte dropped for the bad packet's start byte during resync
		bool bPass = (Packets.Num() == 3);
		bPass = bPass && (BadEndFrames == 1);
		bPass = bPass && (Parser->GetBufferedByteCount() == 3); // Partial packet remains

		if (bPass && Packets.Num() >= 3)
		{
			// Verify packet contents
			bPass = bPass && (Packets[0].Cycle == 258 && Packets[0].Line == 0x03 && Packets[0].Data == 0x04);
			bPass = bPass && (Packets[1].Cycle == 1 && Packets[1].Line == 0xFF && Packets[1].Data == 0xAB);
			bPass = bPass && (Packets[2].Cycle == 4660 && Packets[2].Line == 0x05 && Packets[2].Data == 0x06);
		}

		OutTestLog += FString::Printf(TEXT("  Packets decoded: %d (expected 3)\n"), Packets.Num());
		OutTestLog += FString::Printf(TEXT("  Bytes dropped: %d\n"), BytesDropped);
		OutTestLog += FString::Printf(TEXT("  Bad end frames: %d (expected 1)\n"), BadEndFrames);
		OutTestLog += FString::Printf(TEXT("  Buffered bytes: %d (expected 3)\n"), Parser->GetBufferedByteCount());
		OutTestLog += FString::Printf(TEXT("  Result: %s\n\n"), bPass ? TEXT("PASS") : TEXT("FAIL"));
		bAllPassed = bAllPassed && bPass;
	}

	// Test 2: Split packet across multiple chunks
	{
		OutTestLog += TEXT("Test 2: Split packet handling\n");
		Parser->ResetBuffer();
		Parser->ResetStatistics();

		// First chunk: partial packet
		TArray<uint8> Chunk1;
		Chunk1.Add(0xAA); // START
		Chunk1.Add(0x10); // cycleLo
		Chunk1.Add(0x20); // cycleHi

		TArray<FBenchPacket> Packets1;
		int32 Dropped1 = 0, BadEnd1 = 0;
		Parser->IngestAndParse(Chunk1, Packets1, Dropped1, BadEnd1);

		bool bPass = (Packets1.Num() == 0); // Should have no packets yet
		bPass = bPass && (Parser->GetBufferedByteCount() == 3);

		// Second chunk: rest of packet
		TArray<uint8> Chunk2;
		Chunk2.Add(0x30); // line
		Chunk2.Add(0x40); // data
		Chunk2.Add(0x55); // END

		TArray<FBenchPacket> Packets2;
		int32 Dropped2 = 0, BadEnd2 = 0;
		Parser->IngestAndParse(Chunk2, Packets2, Dropped2, BadEnd2);

		bPass = bPass && (Packets2.Num() == 1);
		if (Packets2.Num() > 0)
		{
			bPass = bPass && (Packets2[0].Cycle == 0x2010);
			bPass = bPass && (Packets2[0].Line == 0x30);
			bPass = bPass && (Packets2[0].Data == 0x40);
		}

		OutTestLog += FString::Printf(TEXT("  Chunk 1 packets: %d (expected 0)\n"), Packets1.Num());
		OutTestLog += FString::Printf(TEXT("  Chunk 2 packets: %d (expected 1)\n"), Packets2.Num());
		if (Packets2.Num() > 0)
		{
			OutTestLog += FString::Printf(TEXT("  Decoded cycle: 0x%04X (expected 0x2010)\n"), Packets2[0].Cycle);
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
		int32 Dropped = 0, BadEnd = 0;
		Parser->IngestAndParse(Junk, Packets, Dropped, BadEnd);

		bool bPass = (Packets.Num() == 0);
		bPass = bPass && (Dropped == 20);
		bPass = bPass && (Parser->GetBufferedByteCount() == 0);

		OutTestLog += FString::Printf(TEXT("  Packets: %d (expected 0)\n"), Packets.Num());
		OutTestLog += FString::Printf(TEXT("  Bytes dropped: %d (expected 20)\n"), Dropped);
		OutTestLog += FString::Printf(TEXT("  Result: %s\n\n"), bPass ? TEXT("PASS") : TEXT("FAIL"));
		bAllPassed = bAllPassed && bPass;
	}

	// Test 4: Consecutive bad end frames
	{
		OutTestLog += TEXT("Test 4: Consecutive bad frames resync\n");
		Parser->ResetBuffer();
		Parser->ResetStatistics();

		TArray<uint8> BadFrames;
		// Bad packet 1
		BadFrames.Add(0xAA);
		BadFrames.Add(0x01);
		BadFrames.Add(0x02);
		BadFrames.Add(0x03);
		BadFrames.Add(0x04);
		BadFrames.Add(0x00); // BAD END

		// Bad packet 2 (overlapping - the 0xAA from previous bad data)
		BadFrames.Add(0xAA);
		BadFrames.Add(0x11);
		BadFrames.Add(0x22);
		BadFrames.Add(0x33);
		BadFrames.Add(0x44);
		BadFrames.Add(0xFF); // BAD END

		// Valid packet after bad frames
		BadFrames.Add(0xAA);
		BadFrames.Add(0x55);
		BadFrames.Add(0x66);
		BadFrames.Add(0x77);
		BadFrames.Add(0x88);
		BadFrames.Add(0x55); // GOOD END

		TArray<FBenchPacket> Packets;
		int32 Dropped = 0, BadEnd = 0;
		Parser->IngestAndParse(BadFrames, Packets, Dropped, BadEnd);

		bool bPass = (Packets.Num() == 1);
		bPass = bPass && (BadEnd == 2);
		if (Packets.Num() > 0)
		{
			bPass = bPass && (Packets[0].Cycle == 0x6655);
			bPass = bPass && (Packets[0].Line == 0x77);
			bPass = bPass && (Packets[0].Data == 0x88);
		}

		OutTestLog += FString::Printf(TEXT("  Packets: %d (expected 1)\n"), Packets.Num());
		OutTestLog += FString::Printf(TEXT("  Bad end frames: %d (expected 2)\n"), BadEnd);
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
		int32 Dropped = 0, BadEnd = 0;
		Parser->IngestAndParse(BigJunk, Packets, Dropped, BadEnd);

		// Buffer should have been trimmed
		bool bPass = (Parser->GetBufferedByteCount() <= Parser->TrimToBytes || Parser->GetBufferedByteCount() == 0);

		OutTestLog += FString::Printf(TEXT("  Buffered after overflow: %d (expected <= %d)\n"),
			Parser->GetBufferedByteCount(), Parser->TrimToBytes);
		OutTestLog += FString::Printf(TEXT("  Result: %s\n\n"), bPass ? TEXT("PASS") : TEXT("FAIL"));
		bAllPassed = bAllPassed && bPass;
	}

	OutTestLog += FString::Printf(TEXT("=== Overall Result: %s ===\n"), bAllPassed ? TEXT("ALL TESTS PASSED") : TEXT("SOME TESTS FAILED"));

	return bAllPassed;
}
