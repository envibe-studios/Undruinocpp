#include "Commands/BlueprintGraph/EventManager.h"
#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "K2Node_Event.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "EditorAssetLibrary.h"

TSharedPtr<FJsonObject> FEventManager::AddEventNode(const TSharedPtr<FJsonObject>& Params)
{
	// Validate parameters
	if (!Params.IsValid())
	{
		return CreateErrorResponse(TEXT("Invalid parameters"));
	}

	// Get required parameters
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	FString EventName;
	if (!Params->TryGetStringField(TEXT("event_name"), EventName))
	{
		return CreateErrorResponse(TEXT("Missing 'event_name' parameter"));
	}

	// Get optional position parameters
	FVector2D Position(0.0f, 0.0f);
	double PosX = 0.0, PosY = 0.0;
	if (Params->TryGetNumberField(TEXT("pos_x"), PosX))
	{
		Position.X = static_cast<float>(PosX);
	}
	if (Params->TryGetNumberField(TEXT("pos_y"), PosY))
	{
		Position.Y = static_cast<float>(PosY);
	}

	// Load the Blueprint
	UBlueprint* Blueprint = LoadBlueprint(BlueprintName);
	if (!Blueprint)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	// Get the event graph (events can only exist in the event graph)
	if (Blueprint->UbergraphPages.Num() == 0)
	{
		return CreateErrorResponse(TEXT("Blueprint has no event graph"));
	}

	UEdGraph* Graph = Blueprint->UbergraphPages[0];
	if (!Graph)
	{
		return CreateErrorResponse(TEXT("Failed to get Blueprint event graph"));
	}

	// Create the event node
	UK2Node_Event* EventNode = CreateEventNode(Graph, EventName, Position);
	if (!EventNode)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Failed to create event node: %s"), *EventName));
	}

	// Notify changes
	Graph->NotifyGraphChanged();
	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

	return CreateSuccessResponse(EventNode);
}

UK2Node_Event* FEventManager::CreateEventNode(UEdGraph* Graph, const FString& EventName, const FVector2D& Position)
{
	if (!Graph)
	{
		return nullptr;
	}

	UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(Graph);
	if (!Blueprint)
	{
		return nullptr;
	}

	// Check for existing event node to avoid duplicates
	UK2Node_Event* ExistingNode = FindExistingEventNode(Graph, EventName);
	if (ExistingNode)
	{
		UE_LOG(LogTemp, Display, TEXT("EventManager: Using existing event node '%s' (ID: %s)"),
			*EventName, *ExistingNode->NodeGuid.ToString());
		return ExistingNode;
	}

	// Create new event node
	UK2Node_Event* EventNode = NewObject<UK2Node_Event>(Graph);
	if (!EventNode)
	{
		UE_LOG(LogTemp, Error, TEXT("EventManager: Failed to create UK2Node_Event object"));
		return nullptr;
	}

	// Handle built-in Actor events (BeginPlay, Tick, etc.)
	// These events use ReceiveBeginPlay/ReceiveTick internally
	if (EventName.Equals(TEXT("ReceiveBeginPlay"), ESearchCase::IgnoreCase) ||
	    EventName.Equals(TEXT("BeginPlay"), ESearchCase::IgnoreCase))
	{
		EventNode->EventReference.SetExternalDelegateMember(FName(TEXT("ReceiveBeginPlay")));
		EventNode->bOverrideFunction = true;
		UE_LOG(LogTemp, Display, TEXT("EventManager: Creating BeginPlay event node"));
	}
	else if (EventName.Equals(TEXT("ReceiveTick"), ESearchCase::IgnoreCase) ||
	         EventName.Equals(TEXT("Tick"), ESearchCase::IgnoreCase))
	{
		EventNode->EventReference.SetExternalDelegateMember(FName(TEXT("ReceiveTick")));
		EventNode->bOverrideFunction = true;
		UE_LOG(LogTemp, Display, TEXT("EventManager: Creating Tick event node"));
	}
	else if (EventName.Equals(TEXT("ReceiveActorBeginOverlap"), ESearchCase::IgnoreCase) ||
	         EventName.Equals(TEXT("ActorBeginOverlap"), ESearchCase::IgnoreCase))
	{
		EventNode->EventReference.SetExternalDelegateMember(FName(TEXT("ReceiveActorBeginOverlap")));
		EventNode->bOverrideFunction = true;
		UE_LOG(LogTemp, Display, TEXT("EventManager: Creating ActorBeginOverlap event node"));
	}
	else if (EventName.Equals(TEXT("ReceiveActorEndOverlap"), ESearchCase::IgnoreCase) ||
	         EventName.Equals(TEXT("ActorEndOverlap"), ESearchCase::IgnoreCase))
	{
		EventNode->EventReference.SetExternalDelegateMember(FName(TEXT("ReceiveActorEndOverlap")));
		EventNode->bOverrideFunction = true;
		UE_LOG(LogTemp, Display, TEXT("EventManager: Creating ActorEndOverlap event node"));
	}
	else if (EventName.Equals(TEXT("ReceiveDestroyed"), ESearchCase::IgnoreCase) ||
	         EventName.Equals(TEXT("Destroyed"), ESearchCase::IgnoreCase))
	{
		EventNode->EventReference.SetExternalDelegateMember(FName(TEXT("ReceiveDestroyed")));
		EventNode->bOverrideFunction = true;
		UE_LOG(LogTemp, Display, TEXT("EventManager: Creating Destroyed event node"));
	}
	else if (EventName.Equals(TEXT("ReceiveHit"), ESearchCase::IgnoreCase) ||
	         EventName.Equals(TEXT("Hit"), ESearchCase::IgnoreCase))
	{
		EventNode->EventReference.SetExternalDelegateMember(FName(TEXT("ReceiveHit")));
		EventNode->bOverrideFunction = true;
		UE_LOG(LogTemp, Display, TEXT("EventManager: Creating Hit event node"));
	}
	else
	{
		// For custom events, try to find the function in the blueprint class
		UClass* BlueprintClass = Blueprint->GeneratedClass;
		if (BlueprintClass)
		{
			UFunction* EventFunction = BlueprintClass->FindFunctionByName(FName(*EventName));
			if (EventFunction)
			{
				EventNode->EventReference.SetExternalMember(FName(*EventName), BlueprintClass);
				UE_LOG(LogTemp, Display, TEXT("EventManager: Creating custom event node '%s'"), *EventName);
			}
			else
			{
				// Treat as a custom event name
				EventNode->CustomFunctionName = FName(*EventName);
				UE_LOG(LogTemp, Display, TEXT("EventManager: Creating new custom event '%s'"), *EventName);
			}
		}
		else
		{
			// No generated class yet, create as custom event
			EventNode->CustomFunctionName = FName(*EventName);
			UE_LOG(LogTemp, Display, TEXT("EventManager: Creating new custom event '%s' (no generated class)"), *EventName);
		}
	}

	// Set position
	EventNode->NodePosX = static_cast<int32>(Position.X);
	EventNode->NodePosY = static_cast<int32>(Position.Y);

	// Add to graph and initialize
	Graph->AddNode(EventNode, true, false);
	EventNode->CreateNewGuid();
	EventNode->PostPlacedNewNode();
	EventNode->AllocateDefaultPins();

	UE_LOG(LogTemp, Display, TEXT("EventManager: Created event node '%s' (ID: %s)"),
		*EventName, *EventNode->NodeGuid.ToString());

	return EventNode;
}

UK2Node_Event* FEventManager::FindExistingEventNode(UEdGraph* Graph, const FString& EventName)
{
	if (!Graph)
	{
		return nullptr;
	}

	// Normalize event name - map friendly names to internal names
	FString InternalName = EventName;
	if (EventName.Equals(TEXT("BeginPlay"), ESearchCase::IgnoreCase))
	{
		InternalName = TEXT("ReceiveBeginPlay");
	}
	else if (EventName.Equals(TEXT("Tick"), ESearchCase::IgnoreCase))
	{
		InternalName = TEXT("ReceiveTick");
	}
	else if (EventName.Equals(TEXT("ActorBeginOverlap"), ESearchCase::IgnoreCase))
	{
		InternalName = TEXT("ReceiveActorBeginOverlap");
	}
	else if (EventName.Equals(TEXT("ActorEndOverlap"), ESearchCase::IgnoreCase))
	{
		InternalName = TEXT("ReceiveActorEndOverlap");
	}
	else if (EventName.Equals(TEXT("Destroyed"), ESearchCase::IgnoreCase))
	{
		InternalName = TEXT("ReceiveDestroyed");
	}
	else if (EventName.Equals(TEXT("Hit"), ESearchCase::IgnoreCase))
	{
		InternalName = TEXT("ReceiveHit");
	}

	for (UEdGraphNode* Node : Graph->Nodes)
	{
		UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node);
		if (EventNode)
		{
			FName MemberName = EventNode->EventReference.GetMemberName();
			// Check both the provided name and the internal name
			if (MemberName.ToString().Equals(EventName, ESearchCase::IgnoreCase) ||
			    MemberName.ToString().Equals(InternalName, ESearchCase::IgnoreCase))
			{
				return EventNode;
			}
			// Also check CustomFunctionName for custom events
			if (!EventNode->CustomFunctionName.IsNone() &&
			    EventNode->CustomFunctionName.ToString().Equals(EventName, ESearchCase::IgnoreCase))
			{
				return EventNode;
			}
		}
	}

	return nullptr;
}

UBlueprint* FEventManager::LoadBlueprint(const FString& BlueprintName)
{
	// Try direct path first
	FString BlueprintPath = BlueprintName;

	// If no path prefix, assume /Game/Blueprints/
	if (!BlueprintPath.StartsWith(TEXT("/")))
	{
		BlueprintPath = TEXT("/Game/Blueprints/") + BlueprintPath;
	}

	// Add .Blueprint suffix if not present
	if (!BlueprintPath.Contains(TEXT(".")))
	{
		BlueprintPath += TEXT(".") + FPaths::GetBaseFilename(BlueprintPath);
	}

	// Try to load the Blueprint
	UBlueprint* BP = LoadObject<UBlueprint>(nullptr, *BlueprintPath);

	// If not found, try with UEditorAssetLibrary
	if (!BP)
	{
		FString AssetPath = BlueprintPath;
		if (UEditorAssetLibrary::DoesAssetExist(AssetPath))
		{
			UObject* Asset = UEditorAssetLibrary::LoadAsset(AssetPath);
			BP = Cast<UBlueprint>(Asset);
		}
	}

	return BP;
}

TSharedPtr<FJsonObject> FEventManager::CreateSuccessResponse(const UK2Node_Event* EventNode)
{
	TSharedPtr<FJsonObject> Response = MakeShareable(new FJsonObject);
	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("node_id"), EventNode->NodeGuid.ToString());
	Response->SetStringField(TEXT("event_name"), EventNode->EventReference.GetMemberName().ToString());
	Response->SetNumberField(TEXT("pos_x"), EventNode->NodePosX);
	Response->SetNumberField(TEXT("pos_y"), EventNode->NodePosY);
	return Response;
}

TSharedPtr<FJsonObject> FEventManager::CreateErrorResponse(const FString& ErrorMessage)
{
	TSharedPtr<FJsonObject> Response = MakeShareable(new FJsonObject);
	Response->SetBoolField(TEXT("success"), false);
	Response->SetStringField(TEXT("error"), ErrorMessage);
	return Response;
}
