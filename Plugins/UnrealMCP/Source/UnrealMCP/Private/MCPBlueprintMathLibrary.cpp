// UnrealMCP Plugin - Blueprint Math Function Library Implementation

#include "MCPBlueprintMathLibrary.h"

int32 UMCPBlueprintMathLibrary::BitwiseShiftLeft(int32 Value, int32 Shift)
{
	// Clamp shift to valid range (0-31 for int32)
	Shift = FMath::Clamp(Shift, 0, 31);
	return Value << Shift;
}

int32 UMCPBlueprintMathLibrary::BitwiseShiftRight(int32 Value, int32 Shift)
{
	// Clamp shift to valid range (0-31 for int32)
	Shift = FMath::Clamp(Shift, 0, 31);
	return Value >> Shift;
}

int32 UMCPBlueprintMathLibrary::BitwiseAnd(int32 A, int32 B)
{
	return A & B;
}

int32 UMCPBlueprintMathLibrary::BitwiseOr(int32 A, int32 B)
{
	return A | B;
}

int32 UMCPBlueprintMathLibrary::BitwiseXor(int32 A, int32 B)
{
	return A ^ B;
}

int32 UMCPBlueprintMathLibrary::BitwiseNot(int32 Value)
{
	return ~Value;
}
