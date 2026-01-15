// Arduino Communication Plugin - Blueprint Function Library

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ArduinoSerialPort.h"
#include "ArduinoTcpClient.h"
#include "ByteStreamPacketParser.h"
#include "ArduinoBlueprintLibrary.generated.h"

/**
 * Blueprint Function Library for Arduino Communication
 * Provides static utility functions for quick Arduino interaction
 */
UCLASS()
class ARDUINOCOMMUNICATION_API UArduinoBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Get a list of available COM ports */
	UFUNCTION(BlueprintCallable, Category = "Arduino|Utility")
	static TArray<FString> GetAvailableComPorts();

	/** Parse a response from Arduino into type and data components */
	UFUNCTION(BlueprintCallable, Category = "Arduino|Utility")
	static void ParseArduinoResponse(const FString& Response, FString& OutType, FString& OutData);

	/** Create a command string with optional parameter */
	UFUNCTION(BlueprintPure, Category = "Arduino|Utility")
	static FString MakeCommand(const FString& Command, const FString& Parameter = TEXT(""));

	/** Check if an IP address is valid */
	UFUNCTION(BlueprintPure, Category = "Arduino|Utility")
	static bool IsValidIPAddress(const FString& IPAddress);

	/** Convert a string to integer safely */
	UFUNCTION(BlueprintPure, Category = "Arduino|Utility")
	static int32 ParseIntFromResponse(const FString& Data, int32 DefaultValue = 0);

	/** Convert a float to a string with specified decimal places */
	UFUNCTION(BlueprintPure, Category = "Arduino|Utility")
	static FString FloatToArduinoString(float Value, int32 DecimalPlaces = 2);

	/** Parse a key=value pair from Arduino response */
	UFUNCTION(BlueprintCallable, Category = "Arduino|Utility")
	static bool ParseKeyValue(const FString& Data, const FString& Key, FString& OutValue);

	/** Create a serial port instance */
	UFUNCTION(BlueprintCallable, Category = "Arduino|Factory", meta = (WorldContext = "WorldContextObject"))
	static UArduinoSerialPort* CreateSerialPort(UObject* WorldContextObject);

	/** Create a TCP client instance */
	UFUNCTION(BlueprintCallable, Category = "Arduino|Factory", meta = (WorldContext = "WorldContextObject"))
	static UArduinoTcpClient* CreateTcpClient(UObject* WorldContextObject);

	/** Create a byte stream packet parser instance */
	UFUNCTION(BlueprintCallable, Category = "Arduino|Factory", meta = (WorldContext = "WorldContextObject"))
	static UByteStreamPacketParser* CreateByteStreamPacketParser(UObject* WorldContextObject);

	// === Packet Parser Test Functions ===

	/**
	 * Run a comprehensive test of the packet parser
	 * Tests junk prefix, partial packets, split packets, and bad end markers
	 * @param WorldContextObject - World context for object creation
	 * @param OutTestLog - Detailed log of test results
	 * @return true if all tests passed
	 */
	UFUNCTION(BlueprintCallable, Category = "Arduino|Testing", meta = (WorldContext = "WorldContextObject"))
	static bool RunPacketParserTest(UObject* WorldContextObject, FString& OutTestLog);

	/**
	 * Create a test byte stream with various edge cases
	 * Useful for manual testing in Blueprint
	 * @return Byte array containing: junk, valid packets, partial packets, bad frames
	 */
	UFUNCTION(BlueprintPure, Category = "Arduino|Testing")
	static TArray<uint8> CreateTestByteStream();

	// === Bitwise Operation Functions ===

	/**
	 * Bitwise Shift Left (<<)
	 * Shifts the bits of Value to the left by Shift positions
	 * @param Value - The integer value to shift
	 * @param Shift - Number of bit positions to shift left (0-31)
	 * @return The result of Value << Shift
	 */
	UFUNCTION(BlueprintPure, Category = "Math|Bitwise", meta = (DisplayName = "Bitwise Shift Left", CompactNodeTitle = "<<", Keywords = "shift left bitwise"))
	static int32 BitwiseShiftLeft(int32 Value, int32 Shift);

	/**
	 * Bitwise Shift Right (>>)
	 * Shifts the bits of Value to the right by Shift positions
	 * @param Value - The integer value to shift
	 * @param Shift - Number of bit positions to shift right (0-31)
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
	static int32 BitwiseAND(int32 A, int32 B);

	/**
	 * Bitwise OR (|)
	 * Performs a bitwise OR operation on two integers
	 * @param A - First integer operand
	 * @param B - Second integer operand
	 * @return The result of A | B
	 */
	UFUNCTION(BlueprintPure, Category = "Math|Bitwise", meta = (DisplayName = "Bitwise OR", CompactNodeTitle = "|", Keywords = "or bitwise"))
	static int32 BitwiseOR(int32 A, int32 B);

	/**
	 * Bitwise XOR (^)
	 * Performs a bitwise XOR (exclusive or) operation on two integers
	 * @param A - First integer operand
	 * @param B - Second integer operand
	 * @return The result of A ^ B
	 */
	UFUNCTION(BlueprintPure, Category = "Math|Bitwise", meta = (DisplayName = "Bitwise XOR", CompactNodeTitle = "^", Keywords = "xor bitwise exclusive"))
	static int32 BitwiseXOR(int32 A, int32 B);

	/**
	 * Bitwise NOT (~)
	 * Performs a bitwise NOT operation (one's complement)
	 * @param Value - The integer value to invert
	 * @return The result of ~Value
	 */
	UFUNCTION(BlueprintPure, Category = "Math|Bitwise", meta = (DisplayName = "Bitwise NOT", CompactNodeTitle = "~", Keywords = "not bitwise complement invert"))
	static int32 BitwiseNOT(int32 Value);
};
