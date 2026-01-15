// Header for creating math nodes (BitwiseShiftLeft, BitwiseShiftRight, BitwiseAnd, BitwiseOr, BitwiseXor, BitwiseNot)

#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraph.h"

class UK2Node;

/**
 * Creator for Unreal Blueprint math operation nodes
 */
class FMathNodeCreator
{
public:
	/**
	 * Creates a Bitwise Shift Left node (K2Node_CallFunction for integer << shift)
	 * @param Graph - The graph to add the node to
	 * @param Params - JSON parameters containing pos_x, pos_y
	 * @return The created node or nullptr on error
	 */
	static UK2Node* CreateBitwiseShiftLeftNode(UEdGraph* Graph, const TSharedPtr<class FJsonObject>& Params);

	/**
	 * Creates a Bitwise Shift Right node (K2Node_CallFunction for integer >> shift)
	 * @param Graph - The graph to add the node to
	 * @param Params - JSON parameters containing pos_x, pos_y
	 * @return The created node or nullptr on error
	 */
	static UK2Node* CreateBitwiseShiftRightNode(UEdGraph* Graph, const TSharedPtr<class FJsonObject>& Params);

	/**
	 * Creates a Bitwise AND node (K2Node_CallFunction for integer & mask)
	 * @param Graph - The graph to add the node to
	 * @param Params - JSON parameters containing pos_x, pos_y
	 * @return The created node or nullptr on error
	 */
	static UK2Node* CreateBitwiseAndNode(UEdGraph* Graph, const TSharedPtr<class FJsonObject>& Params);

	/**
	 * Creates a Bitwise OR node (K2Node_CallFunction for integer | mask)
	 * @param Graph - The graph to add the node to
	 * @param Params - JSON parameters containing pos_x, pos_y
	 * @return The created node or nullptr on error
	 */
	static UK2Node* CreateBitwiseOrNode(UEdGraph* Graph, const TSharedPtr<class FJsonObject>& Params);

	/**
	 * Creates a Bitwise XOR node (K2Node_CallFunction for integer ^ mask)
	 * @param Graph - The graph to add the node to
	 * @param Params - JSON parameters containing pos_x, pos_y
	 * @return The created node or nullptr on error
	 */
	static UK2Node* CreateBitwiseXorNode(UEdGraph* Graph, const TSharedPtr<class FJsonObject>& Params);

	/**
	 * Creates a Bitwise NOT node (K2Node_CallFunction for ~integer)
	 * @param Graph - The graph to add the node to
	 * @param Params - JSON parameters containing pos_x, pos_y
	 * @return The created node or nullptr on error
	 */
	static UK2Node* CreateBitwiseNotNode(UEdGraph* Graph, const TSharedPtr<class FJsonObject>& Params);
};
