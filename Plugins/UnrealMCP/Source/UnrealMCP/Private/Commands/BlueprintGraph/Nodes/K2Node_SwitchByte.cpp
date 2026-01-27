// K2Node_SwitchByte - Implementation

#include "Commands/BlueprintGraph/Nodes/K2Node_SwitchByte.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "EdGraphSchema_K2.h"

#define LOCTEXT_NAMESPACE "K2Node_SwitchByte"

UK2Node_SwitchByte::UK2Node_SwitchByte(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FText UK2Node_SwitchByte::GetTooltipText() const
{
	return LOCTEXT("SwitchByte_Tooltip", "Selects an output based on a byte value");
}

FText UK2Node_SwitchByte::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("SwitchByte_Title", "Switch on Byte");
}

void UK2Node_SwitchByte::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	UClass* ActionKey = GetClass();
	if (ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(NodeSpawner != nullptr);

		ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}

FText UK2Node_SwitchByte::GetMenuCategory() const
{
	return LOCTEXT("SwitchByte_Category", "Flow Control");
}

void UK2Node_SwitchByte::AddPinToSwitchNode()
{
	// Add a new byte value (default to next sequential value or 0)
	uint8 NewValue = 0;
	if (PinValues.Num() > 0)
	{
		// Find the max value and add 1, with wrapping
		uint8 MaxVal = 0;
		for (uint8 Val : PinValues)
		{
			if (Val > MaxVal)
			{
				MaxVal = Val;
			}
		}
		NewValue = (MaxVal < 255) ? (MaxVal + 1) : 0;
	}

	PinValues.Add(NewValue);

	// Create the corresponding pin
	const FName PinName = GetPinNameFromIndex(PinValues.Num() - 1);
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, PinName);
}

FEdGraphPinType UK2Node_SwitchByte::GetPinType() const
{
	FEdGraphPinType PinType;
	PinType.PinCategory = UEdGraphSchema_K2::PC_Byte;
	return PinType;
}

FEdGraphPinType UK2Node_SwitchByte::GetInnerCaseType() const
{
	return GetPinType();
}

FString UK2Node_SwitchByte::GetPinNameGivenIndex(int32 Index) const
{
	if (PinValues.IsValidIndex(Index))
	{
		return FString::Printf(TEXT("%d"), PinValues[Index]);
	}
	return FString::Printf(TEXT("%d"), Index);
}

void UK2Node_SwitchByte::CreateCasePins()
{
	// Create output pins for each case value
	for (int32 Index = 0; Index < PinValues.Num(); ++Index)
	{
		const FName PinName = GetPinNameFromIndex(Index);
		CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, PinName);
	}
}

void UK2Node_SwitchByte::RemovePin(UEdGraphPin* TargetPin)
{
	if (TargetPin)
	{
		// Find the index of this pin in our values array
		const FString PinName = TargetPin->PinName.ToString();

		for (int32 Index = 0; Index < PinValues.Num(); ++Index)
		{
			if (GetPinNameFromIndex(Index).ToString() == PinName)
			{
				PinValues.RemoveAt(Index);
				break;
			}
		}
	}

	// Call parent to actually remove the pin
	Super::RemovePin(TargetPin);
}

void UK2Node_SwitchByte::CreateSelectionPin()
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Byte, TEXT("Selection"));
}

FName UK2Node_SwitchByte::GetPinNameFromIndex(int32 Index) const
{
	if (PinValues.IsValidIndex(Index))
	{
		return FName(*FString::Printf(TEXT("%d"), PinValues[Index]));
	}
	return FName(*FString::Printf(TEXT("Case_%d"), Index));
}

FString UK2Node_SwitchByte::GetExportTextDefaultValue() const
{
	return TEXT("0");
}

FString UK2Node_SwitchByte::GetUniquePinName()
{
	// Find a unique value that doesn't exist in the current pin values
	uint8 NewValue = 0;
	while (PinValues.Contains(NewValue) && NewValue < 255)
	{
		++NewValue;
	}
	return FString::Printf(TEXT("%d"), NewValue);
}

#undef LOCTEXT_NAMESPACE
