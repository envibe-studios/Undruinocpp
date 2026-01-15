// UnrealMCP Plugin - Blueprint Math Function Library for Bitwise Operations

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MCPBlueprintMathLibrary.generated.h"

/**
 * Blueprint Function Library for Bitwise Math Operations
 * Provides static utility functions for bitwise operations in Blueprints
 */
UCLASS()
class UNREALMCP_API UMCPBlueprintMathLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Bitwise Shift Left (<<)
	 * Shifts the bits of Value to the left by Shift positions
	 * @param Value - The integer value to shift
	 * @param Shift - Number of bit positions to shift left
	 * @return The result of Value << Shift
	 */
	UFUNCTION(BlueprintPure, Category = "Math|Bitwise", meta = (DisplayName = "Bitwise Shift Left", CompactNodeTitle = "<<", Keywords = "shift left bitwise"))
	static int32 BitwiseShiftLeft(int32 Value, int32 Shift);

	/**
	 * Bitwise Shift Right (>>)
	 * Shifts the bits of Value to the right by Shift positions
	 * @param Value - The integer value to shift
	 * @param Shift - Number of bit positions to shift right
	 * @return The result of Value >> Shift
	 */
	UFUNCTION(BlueprintPure, Category = "Math|Bitwise", meta = (DisplayName = "Bitwise Shift Right", CompactNodeTitle = ">>", Keywords = "shift right bitwise"))
	static int32 BitwiseShiftRight(int32 Value, int32 Shift);

	/**
	 * Bitwise AND (&)
	 * Performs a bitwise AND operation on two integers
	 * @param A - First integer operand
	 * @param B - Second integer operand (mask)
	 * @return The result of A & B
	 */
	UFUNCTION(BlueprintPure, Category = "Math|Bitwise", meta = (DisplayName = "Bitwise AND", CompactNodeTitle = "&", Keywords = "and bitwise mask"))
	static int32 BitwiseAnd(int32 A, int32 B);

	/**
	 * Bitwise OR (|)
	 * Performs a bitwise OR operation on two integers
	 * @param A - First integer operand
	 * @param B - Second integer operand
	 * @return The result of A | B
	 */
	UFUNCTION(BlueprintPure, Category = "Math|Bitwise", meta = (DisplayName = "Bitwise OR", CompactNodeTitle = "|", Keywords = "or bitwise"))
	static int32 BitwiseOr(int32 A, int32 B);

	/**
	 * Bitwise XOR (^)
	 * Performs a bitwise XOR operation on two integers
	 * @param A - First integer operand
	 * @param B - Second integer operand
	 * @return The result of A ^ B
	 */
	UFUNCTION(BlueprintPure, Category = "Math|Bitwise", meta = (DisplayName = "Bitwise XOR", CompactNodeTitle = "^", Keywords = "xor bitwise exclusive"))
	static int32 BitwiseXor(int32 A, int32 B);

	/**
	 * Bitwise NOT (~)
	 * Performs a bitwise NOT operation (one's complement)
	 * @param Value - The integer value to invert
	 * @return The result of ~Value
	 */
	UFUNCTION(BlueprintPure, Category = "Math|Bitwise", meta = (DisplayName = "Bitwise NOT", CompactNodeTitle = "~", Keywords = "not bitwise complement invert"))
	static int32 BitwiseNot(int32 Value);
};
