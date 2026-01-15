#include "Commands/BlueprintGraph/Nodes/MathNodes.h"
#include "Commands/BlueprintGraph/Nodes/NodeCreatorUtils.h"
#include "K2Node_CallFunction.h"
#include "MCPBlueprintMathLibrary.h"
#include "Json.h"

UK2Node* FMathNodeCreator::CreateBitwiseShiftLeftNode(UEdGraph* Graph, const TSharedPtr<FJsonObject>& Params)
{
	if (!Graph || !Params.IsValid())
	{
		return nullptr;
	}

	UK2Node_CallFunction* ShiftNode = NewObject<UK2Node_CallFunction>(Graph);
	if (!ShiftNode)
	{
		return nullptr;
	}

	UFunction* ShiftFunc = UMCPBlueprintMathLibrary::StaticClass()->FindFunctionByName(
		GET_FUNCTION_NAME_CHECKED(UMCPBlueprintMathLibrary, BitwiseShiftLeft)
	);

	if (!ShiftFunc)
	{
		return nullptr;
	}

	ShiftNode->SetFromFunction(ShiftFunc);

	double PosX, PosY;
	FNodeCreatorUtils::ExtractNodePosition(Params, PosX, PosY);
	ShiftNode->NodePosX = static_cast<int32>(PosX);
	ShiftNode->NodePosY = static_cast<int32>(PosY);

	Graph->AddNode(ShiftNode, true, false);
	FNodeCreatorUtils::InitializeK2Node(ShiftNode, Graph);

	return ShiftNode;
}

UK2Node* FMathNodeCreator::CreateBitwiseShiftRightNode(UEdGraph* Graph, const TSharedPtr<FJsonObject>& Params)
{
	if (!Graph || !Params.IsValid())
	{
		return nullptr;
	}

	UK2Node_CallFunction* ShiftNode = NewObject<UK2Node_CallFunction>(Graph);
	if (!ShiftNode)
	{
		return nullptr;
	}

	UFunction* ShiftFunc = UMCPBlueprintMathLibrary::StaticClass()->FindFunctionByName(
		GET_FUNCTION_NAME_CHECKED(UMCPBlueprintMathLibrary, BitwiseShiftRight)
	);

	if (!ShiftFunc)
	{
		return nullptr;
	}

	ShiftNode->SetFromFunction(ShiftFunc);

	double PosX, PosY;
	FNodeCreatorUtils::ExtractNodePosition(Params, PosX, PosY);
	ShiftNode->NodePosX = static_cast<int32>(PosX);
	ShiftNode->NodePosY = static_cast<int32>(PosY);

	Graph->AddNode(ShiftNode, true, false);
	FNodeCreatorUtils::InitializeK2Node(ShiftNode, Graph);

	return ShiftNode;
}

UK2Node* FMathNodeCreator::CreateBitwiseAndNode(UEdGraph* Graph, const TSharedPtr<FJsonObject>& Params)
{
	if (!Graph || !Params.IsValid())
	{
		return nullptr;
	}

	UK2Node_CallFunction* AndNode = NewObject<UK2Node_CallFunction>(Graph);
	if (!AndNode)
	{
		return nullptr;
	}

	UFunction* AndFunc = UMCPBlueprintMathLibrary::StaticClass()->FindFunctionByName(
		GET_FUNCTION_NAME_CHECKED(UMCPBlueprintMathLibrary, BitwiseAnd)
	);

	if (!AndFunc)
	{
		return nullptr;
	}

	AndNode->SetFromFunction(AndFunc);

	double PosX, PosY;
	FNodeCreatorUtils::ExtractNodePosition(Params, PosX, PosY);
	AndNode->NodePosX = static_cast<int32>(PosX);
	AndNode->NodePosY = static_cast<int32>(PosY);

	Graph->AddNode(AndNode, true, false);
	FNodeCreatorUtils::InitializeK2Node(AndNode, Graph);

	return AndNode;
}

UK2Node* FMathNodeCreator::CreateBitwiseOrNode(UEdGraph* Graph, const TSharedPtr<FJsonObject>& Params)
{
	if (!Graph || !Params.IsValid())
	{
		return nullptr;
	}

	UK2Node_CallFunction* OrNode = NewObject<UK2Node_CallFunction>(Graph);
	if (!OrNode)
	{
		return nullptr;
	}

	UFunction* OrFunc = UMCPBlueprintMathLibrary::StaticClass()->FindFunctionByName(
		GET_FUNCTION_NAME_CHECKED(UMCPBlueprintMathLibrary, BitwiseOr)
	);

	if (!OrFunc)
	{
		return nullptr;
	}

	OrNode->SetFromFunction(OrFunc);

	double PosX, PosY;
	FNodeCreatorUtils::ExtractNodePosition(Params, PosX, PosY);
	OrNode->NodePosX = static_cast<int32>(PosX);
	OrNode->NodePosY = static_cast<int32>(PosY);

	Graph->AddNode(OrNode, true, false);
	FNodeCreatorUtils::InitializeK2Node(OrNode, Graph);

	return OrNode;
}

UK2Node* FMathNodeCreator::CreateBitwiseXorNode(UEdGraph* Graph, const TSharedPtr<FJsonObject>& Params)
{
	if (!Graph || !Params.IsValid())
	{
		return nullptr;
	}

	UK2Node_CallFunction* XorNode = NewObject<UK2Node_CallFunction>(Graph);
	if (!XorNode)
	{
		return nullptr;
	}

	UFunction* XorFunc = UMCPBlueprintMathLibrary::StaticClass()->FindFunctionByName(
		GET_FUNCTION_NAME_CHECKED(UMCPBlueprintMathLibrary, BitwiseXor)
	);

	if (!XorFunc)
	{
		return nullptr;
	}

	XorNode->SetFromFunction(XorFunc);

	double PosX, PosY;
	FNodeCreatorUtils::ExtractNodePosition(Params, PosX, PosY);
	XorNode->NodePosX = static_cast<int32>(PosX);
	XorNode->NodePosY = static_cast<int32>(PosY);

	Graph->AddNode(XorNode, true, false);
	FNodeCreatorUtils::InitializeK2Node(XorNode, Graph);

	return XorNode;
}

UK2Node* FMathNodeCreator::CreateBitwiseNotNode(UEdGraph* Graph, const TSharedPtr<FJsonObject>& Params)
{
	if (!Graph || !Params.IsValid())
	{
		return nullptr;
	}

	UK2Node_CallFunction* NotNode = NewObject<UK2Node_CallFunction>(Graph);
	if (!NotNode)
	{
		return nullptr;
	}

	UFunction* NotFunc = UMCPBlueprintMathLibrary::StaticClass()->FindFunctionByName(
		GET_FUNCTION_NAME_CHECKED(UMCPBlueprintMathLibrary, BitwiseNot)
	);

	if (!NotFunc)
	{
		return nullptr;
	}

	NotNode->SetFromFunction(NotFunc);

	double PosX, PosY;
	FNodeCreatorUtils::ExtractNodePosition(Params, PosX, PosY);
	NotNode->NodePosX = static_cast<int32>(PosX);
	NotNode->NodePosY = static_cast<int32>(PosY);

	Graph->AddNode(NotNode, true, false);
	FNodeCreatorUtils::InitializeK2Node(NotNode, Graph);

	return NotNode;
}
