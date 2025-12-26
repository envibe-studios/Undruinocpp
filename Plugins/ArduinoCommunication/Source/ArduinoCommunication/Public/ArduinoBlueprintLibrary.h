// Arduino Communication Plugin - Blueprint Function Library

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ArduinoSerialPort.h"
#include "ArduinoTcpClient.h"
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
};
