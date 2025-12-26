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
