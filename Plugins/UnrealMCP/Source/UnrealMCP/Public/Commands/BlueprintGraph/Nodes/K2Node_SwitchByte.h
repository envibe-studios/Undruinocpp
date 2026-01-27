// K2Node_SwitchByte - Switch on Byte node for Blueprint graphs

#pragma once

#include "CoreMinimal.h"
#include "K2Node_Switch.h"
#include "K2Node_SwitchByte.generated.h"

/**
 * UK2Node_SwitchByte - Blueprint node for switching execution based on byte values
 *
 * Similar to Switch on Int, but operates on uint8 (byte) values.
 * Users can specify which output pin corresponds to which byte value.
 */
UCLASS()
class UNREALMCP_API UK2Node_SwitchByte : public UK2Node_Switch
{
	GENERATED_BODY()

public:
	UK2Node_SwitchByte();

	/** Array of byte values for each output pin */
	UPROPERTY(EditAnywhere, Category = "PinOptions")
	TArray<uint8> PinValues;

	//~ Begin UEdGraphNode Interface
	virtual FText GetTooltipText() const override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	//~ End UEdGraphNode Interface

	//~ Begin UK2Node Interface
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	virtual FText GetMenuCategory() const override;
	//~ End UK2Node Interface

	//~ Begin UK2Node_Switch Interface
	virtual void AddPinToSwitchNode() override;
	virtual FEdGraphPinType GetPinType() const override;
	virtual FEdGraphPinType GetInnerCaseType() const override;
	virtual FName GetPinNameGivenIndex(int32 Index) const override;
	virtual void CreateCasePins() override;
	virtual void RemovePin(UEdGraphPin* TargetPin) override;
	virtual FName GetUniquePinName() override;
	//~ End UK2Node_Switch Interface

protected:
	virtual void CreateSelectionPin() override;
};
