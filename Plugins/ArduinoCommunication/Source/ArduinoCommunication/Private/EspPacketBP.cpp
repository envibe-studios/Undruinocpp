// Arduino Communication Plugin - ESP Packet Blueprint Library Implementation

#include "EspPacketBP.h"

// ============================================================================
// Low-Level Unpacking Helpers
// ============================================================================

int32 UEspPacketBP::ReadUInt16LE(const TArray<uint8>& Bytes, int32 Offset, bool& bOk)
{
	// Need at least 2 bytes from offset
	if (Offset < 0 || Offset + 2 > Bytes.Num())
	{
		bOk = false;
		return 0;
	}

	bOk = true;
	// Little-endian: low byte first, high byte second
	return static_cast<int32>(Bytes[Offset]) | (static_cast<int32>(Bytes[Offset + 1]) << 8);
}

int32 UEspPacketBP::ReadInt16LE(const TArray<uint8>& Bytes, int32 Offset, bool& bOk)
{
	// Need at least 2 bytes from offset
	if (Offset < 0 || Offset + 2 > Bytes.Num())
	{
		bOk = false;
		return 0;
	}

	bOk = true;
	// Little-endian: low byte first, high byte second
	uint16 Raw = static_cast<uint16>(Bytes[Offset]) | (static_cast<uint16>(Bytes[Offset + 1]) << 8);
	// Convert to signed using proper casting
	return static_cast<int32>(static_cast<int16>(Raw));
}

int64 UEspPacketBP::ReadUInt32LE(const TArray<uint8>& Bytes, int32 Offset, bool& bOk)
{
	// Need at least 4 bytes from offset
	if (Offset < 0 || Offset + 4 > Bytes.Num())
	{
		bOk = false;
		return 0;
	}

	bOk = true;
	// Little-endian: least significant byte first
	return static_cast<int64>(Bytes[Offset])
		| (static_cast<int64>(Bytes[Offset + 1]) << 8)
		| (static_cast<int64>(Bytes[Offset + 2]) << 16)
		| (static_cast<int64>(Bytes[Offset + 3]) << 24);
}

FString UEspPacketBP::BytesToHexString(const TArray<uint8>& Bytes)
{
	if (Bytes.Num() == 0)
	{
		return TEXT("");
	}

	FString Result;
	Result.Reserve(Bytes.Num() * 3); // "XX " per byte

	for (int32 i = 0; i < Bytes.Num(); ++i)
	{
		if (i > 0)
		{
			Result += TEXT(" ");
		}
		Result += FString::Printf(TEXT("%02X"), Bytes[i]);
	}

	return Result;
}

EEspMsgType UEspPacketBP::ByteToMsgType(uint8 TypeByte)
{
	switch (TypeByte)
	{
	case 1: return EEspMsgType::WheelTurn;
	case 2: return EEspMsgType::RepairProgress;
	case 3: return EEspMsgType::JackState;
	case 4: return EEspMsgType::WeaponTag;
	case 5: return EEspMsgType::ReloadTag;
	case 6: return EEspMsgType::WeaponImu;
	default: return EEspMsgType::None;
	}
}

// ============================================================================
// Per-Type Payload Parsers
// ============================================================================

bool UEspPacketBP::ParseWheelTurnPayload(const TArray<uint8>& Payload, FWheelTurnData& OutData)
{
	// Expected Len = 2: [WheelIndex, Direction]
	if (Payload.Num() != 2)
	{
		OutData = FWheelTurnData();
		return false;
	}

	OutData.WheelIndex = Payload[0];
	OutData.bRight = (Payload[1] != 0);
	return true;
}

bool UEspPacketBP::ParseRepairProgressPayload(const TArray<uint8>& Payload, FRepairProgressData& OutData)
{
	// Expected Len = 2: [Amount_L, Amount_H] (uint16 LE)
	if (Payload.Num() != 2)
	{
		OutData = FRepairProgressData();
		return false;
	}

	bool bOk;
	int32 Amount = ReadUInt16LE(Payload, 0, bOk);
	if (!bOk)
	{
		OutData = FRepairProgressData();
		return false;
	}

	OutData.Amount = Amount;
	return true;
}

bool UEspPacketBP::ParseJackStatePayload(const TArray<uint8>& Payload, FJackStateData& OutData)
{
	// Expected Len = 1: [State]
	if (Payload.Num() != 1)
	{
		OutData = FJackStateData();
		return false;
	}

	OutData.State = Payload[0];
	return true;
}

bool UEspPacketBP::ParseWeaponTagPayload(const TArray<uint8>& Payload, FWeaponTagData& OutData)
{
	// Expected Len = 6: [Side, UID (4 bytes LE), Button]
	if (Payload.Num() != 6)
	{
		OutData = FWeaponTagData();
		return false;
	}

	OutData.Side = Payload[0];

	// Read UID from offset 1 (4 bytes LE)
	bool bOk;
	OutData.UID = ReadUInt32LE(Payload, 1, bOk);
	if (!bOk)
	{
		OutData = FWeaponTagData();
		return false;
	}

	OutData.Button = Payload[5];
	return true;
}

bool UEspPacketBP::ParseReloadTagPayload(const TArray<uint8>& Payload, FReloadTagData& OutData)
{
	// Expected Len = 4: [UID (4 bytes LE)]
	if (Payload.Num() != 4)
	{
		OutData = FReloadTagData();
		return false;
	}

	bool bOk;
	OutData.UID = ReadUInt32LE(Payload, 0, bOk);
	if (!bOk)
	{
		OutData = FReloadTagData();
		return false;
	}

	return true;
}

bool UEspPacketBP::ParseWeaponImuPayload(const TArray<uint8>& Payload, FWeaponImuData& OutData)
{
	// Expected Len = 6: [Side, Pitch_L, Pitch_H, Yaw_L, Yaw_H, Buttons]
	if (Payload.Num() != 6)
	{
		OutData = FWeaponImuData();
		return false;
	}

	OutData.Side = Payload[0];

	// Read signed int16 for Pitch (offset 1)
	bool bOkPitch;
	OutData.Pitch = ReadInt16LE(Payload, 1, bOkPitch);

	// Read signed int16 for Yaw (offset 3)
	bool bOkYaw;
	OutData.Yaw = ReadInt16LE(Payload, 3, bOkYaw);

	if (!bOkPitch || !bOkYaw)
	{
		OutData = FWeaponImuData();
		return false;
	}

	OutData.Buttons = Payload[5];
	return true;
}
