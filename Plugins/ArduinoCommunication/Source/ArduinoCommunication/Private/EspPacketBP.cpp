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
	// Expected Len = 6: [Side, UID (4 bytes LE), Present]
	// Side: 0 = PORT, 1 = STARBOARD
	// Present: 1 = inserted, 0 = removed
	if (Payload.Num() != 6)
	{
		UE_LOG(LogTemp, Warning, TEXT("WEAPON_TAG (Type 4) dropped: unexpected LEN=%d, expected 6"), Payload.Num());
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

	// Present byte: 1 = inserted, 0 = removed
	OutData.bPresent = (Payload[5] != 0);
	return true;
}

bool UEspPacketBP::ParseReloadTagPayload(const TArray<uint8>& Payload, FReloadTagData& OutData)
{
	// Expected Len = 5: [UID (4 bytes LE), Present (1 byte)]
	if (Payload.Num() != 5)
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

	// Present byte: 1 = inserted, 0 = removed
	OutData.bPresent = (Payload[4] != 0);

	return true;
}

bool UEspPacketBP::ParseWeaponImuPayload(const TArray<uint8>& Payload, FWeaponImuData& OutData)
{
	// Expected Len = 10: [Side, qx_i16 LE, qy_i16 LE, qz_i16 LE, qw_i16 LE, Buttons]
	// Side: 0 = PORT, 1 = STARBOARD
	// Quaternion components: int16 values divided by 32767 to get float range [-1.0, 1.0]
	// Buttons: bitfield (bit0 = trigger pressed)
	if (Payload.Num() != 10)
	{
		UE_LOG(LogTemp, Warning, TEXT("WEAPON_IMU (Type 6) dropped: unexpected LEN=%d, expected 10"), Payload.Num());
		OutData = FWeaponImuData();
		return false;
	}

	OutData.Side = Payload[0];

	// Read signed int16 for each quaternion component and convert to float
	constexpr float QuatScale = 32767.0f;

	bool bOkQx, bOkQy, bOkQz, bOkQw;
	int32 RawQx = ReadInt16LE(Payload, 1, bOkQx);
	int32 RawQy = ReadInt16LE(Payload, 3, bOkQy);
	int32 RawQz = ReadInt16LE(Payload, 5, bOkQz);
	int32 RawQw = ReadInt16LE(Payload, 7, bOkQw);

	if (!bOkQx || !bOkQy || !bOkQz || !bOkQw)
	{
		OutData = FWeaponImuData();
		return false;
	}

	// Convert int16 to normalized float by dividing by 32767
	float Qx = static_cast<float>(RawQx) / QuatScale;
	float Qy = static_cast<float>(RawQy) / QuatScale;
	float Qz = static_cast<float>(RawQz) / QuatScale;
	float Qw = static_cast<float>(RawQw) / QuatScale;

	// Use SetFromQuaternion to set quaternion as source-of-truth and derive unwound Euler angles
	FQuat Quat(Qx, Qy, Qz, Qw);
	OutData.SetFromQuaternion(Quat);

	OutData.Buttons = Payload[9];
	return true;
}

FVector UEspPacketBP::SmoothEulerAngles(const FVector& NewAngles, const FVector& PreviousAngles)
{
	// Apply shortest-path smoothing by finding the delta that minimizes wrap-around jumps
	// For each axis, if the difference exceeds 180 degrees, adjust by 360 degrees

	auto ShortestPathAngle = [](float NewAngle, float PreviousAngle) -> float
	{
		// Compute the difference
		float Delta = NewAngle - PreviousAngle;

		// If delta is greater than 180, we wrapped around and should go the other way
		while (Delta > 180.0f)
		{
			Delta -= 360.0f;
		}
		while (Delta < -180.0f)
		{
			Delta += 360.0f;
		}

		// Return the previous angle plus the shortest delta
		return PreviousAngle + Delta;
	};

	return FVector(
		ShortestPathAngle(NewAngles.X, PreviousAngles.X),  // Pitch
		ShortestPathAngle(NewAngles.Y, PreviousAngles.Y),  // Yaw
		ShortestPathAngle(NewAngles.Z, PreviousAngles.Z)   // Roll
	);
}

bool UEspPacketBP::ParseWeaponImuPayloadSmoothed(const TArray<uint8>& Payload, const FWeaponImuData& PreviousData, FWeaponImuData& OutData)
{
	// First parse normally
	if (!ParseWeaponImuPayload(Payload, OutData))
	{
		return false;
	}

	// Apply shortest-path smoothing to Euler angles relative to previous frame
	OutData.EulerAngles = SmoothEulerAngles(OutData.EulerAngles, PreviousData.EulerAngles);
	return true;
}
