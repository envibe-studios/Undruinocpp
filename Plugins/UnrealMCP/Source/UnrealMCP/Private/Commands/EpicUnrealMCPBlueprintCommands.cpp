#include "Commands/EpicUnrealMCPBlueprintCommands.h"
#include "Commands/EpicUnrealMCPCommonUtils.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Factories/BlueprintFactory.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_Event.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceConstant.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Engine/Engine.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "UObject/Field.h"
#include "UObject/FieldPath.h"
#include "EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "PackageTools.h"

FEpicUnrealMCPBlueprintCommands::FEpicUnrealMCPBlueprintCommands()
{
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    if (CommandType == TEXT("create_blueprint"))
    {
        return HandleCreateBlueprint(Params);
    }
    else if (CommandType == TEXT("add_component_to_blueprint"))
    {
        return HandleAddComponentToBlueprint(Params);
    }
    else if (CommandType == TEXT("set_physics_properties"))
    {
        return HandleSetPhysicsProperties(Params);
    }
    else if (CommandType == TEXT("compile_blueprint"))
    {
        return HandleCompileBlueprint(Params);
    }
    else if (CommandType == TEXT("set_static_mesh_properties"))
    {
        return HandleSetStaticMeshProperties(Params);
    }
    else if (CommandType == TEXT("spawn_blueprint_actor"))
    {
        return HandleSpawnBlueprintActor(Params);
    }
    else if (CommandType == TEXT("set_mesh_material_color"))
    {
        return HandleSetMeshMaterialColor(Params);
    }
    // Material management commands
    else if (CommandType == TEXT("get_available_materials"))
    {
        return HandleGetAvailableMaterials(Params);
    }
    else if (CommandType == TEXT("apply_material_to_actor"))
    {
        return HandleApplyMaterialToActor(Params);
    }
    else if (CommandType == TEXT("apply_material_to_blueprint"))
    {
        return HandleApplyMaterialToBlueprint(Params);
    }
    else if (CommandType == TEXT("get_actor_material_info"))
    {
        return HandleGetActorMaterialInfo(Params);
    }
    else if (CommandType == TEXT("get_blueprint_material_info"))
    {
        return HandleGetBlueprintMaterialInfo(Params);
    }
    // Blueprint analysis commands
    else if (CommandType == TEXT("read_blueprint_content"))
    {
        return HandleReadBlueprintContent(Params);
    }
    else if (CommandType == TEXT("analyze_blueprint_graph"))
    {
        return HandleAnalyzeBlueprintGraph(Params);
    }
    else if (CommandType == TEXT("get_blueprint_variable_details"))
    {
        return HandleGetBlueprintVariableDetails(Params);
    }
    else if (CommandType == TEXT("get_blueprint_function_details"))
    {
        return HandleGetBlueprintFunctionDetails(Params);
    }
    else if (CommandType == TEXT("get_blueprint_component_hierarchy"))
    {
        return HandleGetBlueprintComponentHierarchy(Params);
    }
    else if (CommandType == TEXT("fix_blueprint_scaled_root"))
    {
        return HandleFixBlueprintScaledRoot(Params);
    }
    else if (CommandType == TEXT("correct_blueprint_relative_bake"))
    {
        return HandleCorrectBlueprintRelativeBake(Params);
    }
    else if (CommandType == TEXT("fix_blueprint_physics_root"))
    {
        return HandleFixBlueprintPhysicsRoot(Params);
    }
    else if (CommandType == TEXT("set_blueprint_component_transform"))
    {
        return HandleSetBlueprintComponentTransform(Params);
    }
    else if (CommandType == TEXT("set_blueprint_component_absolute"))
    {
        return HandleSetBlueprintComponentAbsolute(Params);
    }
    else if (CommandType == TEXT("reparent_blueprint_component"))
    {
        return HandleReparentBlueprintComponent(Params);
    }

    return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown blueprint command: %s"), *CommandType));
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleCreateBlueprint(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("name"), BlueprintName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    // Get optional folder path parameter, default to /Game/Blueprints/
    FString PackagePath;
    if (!Params->TryGetStringField(TEXT("folder_path"), PackagePath))
    {
        PackagePath = TEXT("/Game/Blueprints/");
    }

    // Ensure the path starts with /Game/ and ends with /
    if (!PackagePath.StartsWith(TEXT("/Game/")))
    {
        PackagePath = TEXT("/Game/") + PackagePath;
    }
    if (!PackagePath.EndsWith(TEXT("/")))
    {
        PackagePath += TEXT("/");
    }

    FString AssetName = BlueprintName;
    if (UEditorAssetLibrary::DoesAssetExist(PackagePath + AssetName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint already exists: %s"), *BlueprintName));
    }

    // Create the blueprint factory
    UBlueprintFactory* Factory = NewObject<UBlueprintFactory>();
    
    // Handle parent class
    FString ParentClass;
    Params->TryGetStringField(TEXT("parent_class"), ParentClass);
    
    // Default to Actor if no parent class specified
    UClass* SelectedParentClass = AActor::StaticClass();
    
    // Try to find the specified parent class
    if (!ParentClass.IsEmpty())
    {
        FString ClassName = ParentClass;
        if (!ClassName.StartsWith(TEXT("A")))
        {
            ClassName = TEXT("A") + ClassName;
        }
        
        // First try direct StaticClass lookup for common classes
        UClass* FoundClass = nullptr;
        if (ClassName == TEXT("APawn"))
        {
            FoundClass = APawn::StaticClass();
        }
        else if (ClassName == TEXT("AActor"))
        {
            FoundClass = AActor::StaticClass();
        }
        else
        {
            // Try loading the class using LoadClass which is more reliable than FindObject
            const FString ClassPath = FString::Printf(TEXT("/Script/Engine.%s"), *ClassName);
            FoundClass = LoadClass<AActor>(nullptr, *ClassPath);
            
            if (!FoundClass)
            {
                // Try alternate paths if not found
                const FString GameClassPath = FString::Printf(TEXT("/Script/Game.%s"), *ClassName);
                FoundClass = LoadClass<AActor>(nullptr, *GameClassPath);
            }
        }

        if (FoundClass)
        {
            SelectedParentClass = FoundClass;
            UE_LOG(LogTemp, Log, TEXT("Successfully set parent class to '%s'"), *ClassName);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Could not find specified parent class '%s' at paths: /Script/Engine.%s or /Script/Game.%s, defaulting to AActor"), 
                *ClassName, *ClassName, *ClassName);
        }
    }
    
    Factory->ParentClass = SelectedParentClass;

    // Create the blueprint
    UPackage* Package = CreatePackage(*(PackagePath + AssetName));
    UBlueprint* NewBlueprint = Cast<UBlueprint>(Factory->FactoryCreateNew(UBlueprint::StaticClass(), Package, *AssetName, RF_Standalone | RF_Public, nullptr, GWarn));

    if (NewBlueprint)
    {
        // Notify the asset registry
        FAssetRegistryModule::AssetCreated(NewBlueprint);

        // Mark the package dirty
        Package->MarkPackageDirty();

        TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
        ResultObj->SetStringField(TEXT("name"), AssetName);
        ResultObj->SetStringField(TEXT("path"), PackagePath + AssetName);
        return ResultObj;
    }

    return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create blueprint"));
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleAddComponentToBlueprint(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString ComponentType;
    if (!Params->TryGetStringField(TEXT("component_type"), ComponentType))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'type' parameter"));
    }

    FString ComponentName;
    if (!Params->TryGetStringField(TEXT("component_name"), ComponentName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    // Find the blueprint
    UBlueprint* Blueprint = FEpicUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Create the component - dynamically find the component class by name
    UClass* ComponentClass = nullptr;

    // Try to find the class with exact name first
    ComponentClass = FindObject<UClass>(nullptr, *ComponentType);
    
    // If not found, try with "Component" suffix
    if (!ComponentClass && !ComponentType.EndsWith(TEXT("Component")))
    {
        FString ComponentTypeWithSuffix = ComponentType + TEXT("Component");
        ComponentClass = FindObject<UClass>(nullptr, *ComponentTypeWithSuffix);
    }
    
    // If still not found, try with "U" prefix
    if (!ComponentClass && !ComponentType.StartsWith(TEXT("U")))
    {
        FString ComponentTypeWithPrefix = TEXT("U") + ComponentType;
        ComponentClass = FindObject<UClass>(nullptr, *ComponentTypeWithPrefix);
        
        // Try with both prefix and suffix
        if (!ComponentClass && !ComponentType.EndsWith(TEXT("Component")))
        {
            FString ComponentTypeWithBoth = TEXT("U") + ComponentType + TEXT("Component");
            ComponentClass = FindObject<UClass>(nullptr, *ComponentTypeWithBoth);
        }
    }
    
    // Verify that the class is a valid component type
    if (!ComponentClass || !ComponentClass->IsChildOf(UActorComponent::StaticClass()))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown component type: %s"), *ComponentType));
    }

    // Add the component to the blueprint
    USCS_Node* NewNode = Blueprint->SimpleConstructionScript->CreateNode(ComponentClass, *ComponentName);
    if (NewNode)
    {
        // Set transform if provided
        USceneComponent* SceneComponent = Cast<USceneComponent>(NewNode->ComponentTemplate);
        if (SceneComponent)
        {
            if (Params->HasField(TEXT("location")))
            {
                SceneComponent->SetRelativeLocation(FEpicUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("location")));
            }
            if (Params->HasField(TEXT("rotation")))
            {
                SceneComponent->SetRelativeRotation(FEpicUnrealMCPCommonUtils::GetRotatorFromJson(Params, TEXT("rotation")));
            }
            if (Params->HasField(TEXT("scale")))
            {
                SceneComponent->SetRelativeScale3D(FEpicUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("scale")));
            }
        }

        // Add to root if no parent specified
        Blueprint->SimpleConstructionScript->AddNode(NewNode);

        // Compile the blueprint
        FKismetEditorUtilities::CompileBlueprint(Blueprint);

        TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
        ResultObj->SetStringField(TEXT("component_name"), ComponentName);
        ResultObj->SetStringField(TEXT("component_type"), ComponentType);
        return ResultObj;
    }

    return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to add component to blueprint"));
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleSetPhysicsProperties(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString ComponentName;
    if (!Params->TryGetStringField(TEXT("component_name"), ComponentName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'component_name' parameter"));
    }

    // Find the blueprint
    UBlueprint* Blueprint = FEpicUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Find the component
    USCS_Node* ComponentNode = nullptr;
    for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
    {
        if (Node && Node->GetVariableName().ToString() == ComponentName)
        {
            ComponentNode = Node;
            break;
        }
    }

    if (!ComponentNode)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Component not found: %s"), *ComponentName));
    }

    UPrimitiveComponent* PrimComponent = Cast<UPrimitiveComponent>(ComponentNode->ComponentTemplate);
    if (!PrimComponent)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Component is not a primitive component"));
    }

    // Set physics properties
    if (Params->HasField(TEXT("simulate_physics")))
    {
        PrimComponent->SetSimulatePhysics(Params->GetBoolField(TEXT("simulate_physics")));
    }

    if (Params->HasField(TEXT("mass")))
    {
        float Mass = Params->GetNumberField(TEXT("mass"));
        // In UE5.5, use proper overrideMass instead of just scaling
        PrimComponent->SetMassOverrideInKg(NAME_None, Mass);
        UE_LOG(LogTemp, Display, TEXT("Set mass for component %s to %f kg"), *ComponentName, Mass);
    }

    if (Params->HasField(TEXT("linear_damping")))
    {
        PrimComponent->SetLinearDamping(Params->GetNumberField(TEXT("linear_damping")));
    }

    if (Params->HasField(TEXT("angular_damping")))
    {
        PrimComponent->SetAngularDamping(Params->GetNumberField(TEXT("angular_damping")));
    }

    // Mark the blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("component"), ComponentName);
    return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleCompileBlueprint(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    const bool bReloadFromDisk = Params->HasField(TEXT("reload_from_disk")) && Params->GetBoolField(TEXT("reload_from_disk"));

    // Find the blueprint
    UBlueprint* Blueprint = FEpicUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    if (bReloadFromDisk)
    {
        UPackage* Package = Blueprint->GetOutermost();
        if (!Package)
        {
            return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Blueprint has no package to reload"));
        }

        // Discard in-memory edits and reload the on-disk asset (e.g. after git restore).
        TArray<UPackage*> PackagesToReload;
        PackagesToReload.Add(Package);
        UPackageTools::ReloadPackages(PackagesToReload);

        Blueprint = FEpicUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
        if (!Blueprint)
        {
            return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found after reload: %s"), *BlueprintName));
        }
    }

    // Compile the blueprint
    FKismetEditorUtilities::CompileBlueprint(Blueprint);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("name"), BlueprintName);
    ResultObj->SetBoolField(TEXT("compiled"), true);
    ResultObj->SetBoolField(TEXT("reloaded_from_disk"), bReloadFromDisk);
    return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleSpawnBlueprintActor(const TSharedPtr<FJsonObject>& Params)
{
    UE_LOG(LogTemp, Warning, TEXT("HandleSpawnBlueprintActor: Starting blueprint actor spawn"));
    
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        UE_LOG(LogTemp, Error, TEXT("HandleSpawnBlueprintActor: Missing blueprint_name parameter"));
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString ActorName;
    if (!Params->TryGetStringField(TEXT("actor_name"), ActorName))
    {
        UE_LOG(LogTemp, Error, TEXT("HandleSpawnBlueprintActor: Missing actor_name parameter"));
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'actor_name' parameter"));
    }

    UE_LOG(LogTemp, Warning, TEXT("HandleSpawnBlueprintActor: Looking for blueprint '%s'"), *BlueprintName);

    // Find the blueprint
    UBlueprint* Blueprint = FEpicUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        UE_LOG(LogTemp, Error, TEXT("HandleSpawnBlueprintActor: Blueprint not found: %s"), *BlueprintName);
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    UE_LOG(LogTemp, Warning, TEXT("HandleSpawnBlueprintActor: Blueprint found, getting transform parameters"));

    // Get transform parameters
    FVector Location(0.0f, 0.0f, 0.0f);
    FRotator Rotation(0.0f, 0.0f, 0.0f);

    if (Params->HasField(TEXT("location")))
    {
        Location = FEpicUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("location"));
        UE_LOG(LogTemp, Warning, TEXT("HandleSpawnBlueprintActor: Location set to (%f, %f, %f)"), Location.X, Location.Y, Location.Z);
    }
    if (Params->HasField(TEXT("rotation")))
    {
        Rotation = FEpicUnrealMCPCommonUtils::GetRotatorFromJson(Params, TEXT("rotation"));
        UE_LOG(LogTemp, Warning, TEXT("HandleSpawnBlueprintActor: Rotation set to (%f, %f, %f)"), Rotation.Pitch, Rotation.Yaw, Rotation.Roll);
    }

    UE_LOG(LogTemp, Warning, TEXT("HandleSpawnBlueprintActor: Getting editor world"));

    // Spawn the actor
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("HandleSpawnBlueprintActor: Failed to get editor world"));
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get editor world"));
    }

    UE_LOG(LogTemp, Warning, TEXT("HandleSpawnBlueprintActor: Creating spawn transform"));

    FTransform SpawnTransform;
    SpawnTransform.SetLocation(Location);
    SpawnTransform.SetRotation(FQuat(Rotation));

    // Add a small delay to allow the engine to process the newly compiled class
    FPlatformProcess::Sleep(0.2f);

    UE_LOG(LogTemp, Warning, TEXT("HandleSpawnBlueprintActor: About to spawn actor from blueprint '%s' with GeneratedClass: %s"), 
           *BlueprintName, Blueprint->GeneratedClass ? *Blueprint->GeneratedClass->GetName() : TEXT("NULL"));

    AActor* NewActor = World->SpawnActor<AActor>(Blueprint->GeneratedClass, SpawnTransform);
    
    UE_LOG(LogTemp, Warning, TEXT("HandleSpawnBlueprintActor: SpawnActor completed, NewActor: %s"), 
           NewActor ? *NewActor->GetName() : TEXT("NULL"));
    
    if (NewActor)
    {
        UE_LOG(LogTemp, Warning, TEXT("HandleSpawnBlueprintActor: Setting actor label to '%s'"), *ActorName);
        NewActor->SetActorLabel(*ActorName);
        
        UE_LOG(LogTemp, Warning, TEXT("HandleSpawnBlueprintActor: About to convert actor to JSON"));
        TSharedPtr<FJsonObject> Result = FEpicUnrealMCPCommonUtils::ActorToJsonObject(NewActor, true);
        
        UE_LOG(LogTemp, Warning, TEXT("HandleSpawnBlueprintActor: JSON conversion completed, returning result"));
        return Result;
    }

    UE_LOG(LogTemp, Error, TEXT("HandleSpawnBlueprintActor: Failed to spawn blueprint actor"));
    return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to spawn blueprint actor"));
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleSetStaticMeshProperties(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString ComponentName;
    if (!Params->TryGetStringField(TEXT("component_name"), ComponentName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'component_name' parameter"));
    }

    // Find the blueprint
    UBlueprint* Blueprint = FEpicUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Find the component
    USCS_Node* ComponentNode = nullptr;
    for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
    {
        if (Node && Node->GetVariableName().ToString() == ComponentName)
        {
            ComponentNode = Node;
            break;
        }
    }

    if (!ComponentNode)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Component not found: %s"), *ComponentName));
    }

    UStaticMeshComponent* MeshComponent = Cast<UStaticMeshComponent>(ComponentNode->ComponentTemplate);
    if (!MeshComponent)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Component is not a static mesh component"));
    }

    // Set static mesh properties
    if (Params->HasField(TEXT("static_mesh")))
    {
        FString MeshPath = Params->GetStringField(TEXT("static_mesh"));
        UStaticMesh* Mesh = Cast<UStaticMesh>(UEditorAssetLibrary::LoadAsset(MeshPath));
        if (Mesh)
        {
            MeshComponent->SetStaticMesh(Mesh);
        }
    }

    if (Params->HasField(TEXT("material")))
    {
        FString MaterialPath = Params->GetStringField(TEXT("material"));
        UMaterialInterface* Material = Cast<UMaterialInterface>(UEditorAssetLibrary::LoadAsset(MaterialPath));
        if (Material)
        {
            MeshComponent->SetMaterial(0, Material);
        }
    }

    // Mark the blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("component"), ComponentName);
    return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleSetMeshMaterialColor(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString ComponentName;
    if (!Params->TryGetStringField(TEXT("component_name"), ComponentName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'component_name' parameter"));
    }

    // Find the blueprint
    UBlueprint* Blueprint = FEpicUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Find the component
    USCS_Node* ComponentNode = nullptr;
    for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
    {
        if (Node && Node->GetVariableName().ToString() == ComponentName)
        {
            ComponentNode = Node;
            break;
        }
    }

    if (!ComponentNode)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Component not found: %s"), *ComponentName));
    }

    // Try to cast to StaticMeshComponent or PrimitiveComponent
    UPrimitiveComponent* PrimComponent = Cast<UPrimitiveComponent>(ComponentNode->ComponentTemplate);
    if (!PrimComponent)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Component is not a primitive component"));
    }

    // Get color parameter
    TArray<float> ColorArray;
    const TArray<TSharedPtr<FJsonValue>>* ColorJsonArray;
    if (!Params->TryGetArrayField(TEXT("color"), ColorJsonArray) || ColorJsonArray->Num() != 4)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("'color' must be an array of 4 float values [R, G, B, A]"));
    }

    for (const TSharedPtr<FJsonValue>& Value : *ColorJsonArray)
    {
        ColorArray.Add(FMath::Clamp(Value->AsNumber(), 0.0f, 1.0f));
    }

    FLinearColor Color(ColorArray[0], ColorArray[1], ColorArray[2], ColorArray[3]);

    // Get material slot index
    int32 MaterialSlot = 0;
    if (Params->HasField(TEXT("material_slot")))
    {
        MaterialSlot = Params->GetIntegerField(TEXT("material_slot"));
    }

    // Get parameter name
    FString ParameterName = TEXT("BaseColor");
    Params->TryGetStringField(TEXT("parameter_name"), ParameterName);

    // Get or create material
    UMaterialInterface* Material = nullptr;
    
    // Check if a specific material path was provided
    FString MaterialPath;
    if (Params->TryGetStringField(TEXT("material_path"), MaterialPath))
    {
        Material = Cast<UMaterialInterface>(UEditorAssetLibrary::LoadAsset(MaterialPath));
        if (!Material)
        {
            return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load material: %s"), *MaterialPath));
        }
    }
    else
    {
        // Use existing material on the component
        Material = PrimComponent->GetMaterial(MaterialSlot);
        if (!Material)
        {
            // Try to use a default material
            Material = Cast<UMaterialInterface>(UEditorAssetLibrary::LoadAsset(TEXT("/Engine/BasicShapes/BasicShapeMaterial")));
            if (!Material)
            {
                return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No material found on component and failed to load default material"));
            }
        }
    }

    // Create a dynamic material instance
    UMaterialInstanceDynamic* DynMaterial = UMaterialInstanceDynamic::Create(Material, PrimComponent);
    if (!DynMaterial)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create dynamic material instance"));
    }

    // Set the color parameter
    DynMaterial->SetVectorParameterValue(*ParameterName, Color);

    // Apply the material to the component
    PrimComponent->SetMaterial(MaterialSlot, DynMaterial);

    // Mark the blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    // Log success
    UE_LOG(LogTemp, Log, TEXT("Successfully set material color on component %s: R=%f, G=%f, B=%f, A=%f"), 
        *ComponentName, Color.R, Color.G, Color.B, Color.A);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("component"), ComponentName);
    ResultObj->SetNumberField(TEXT("material_slot"), MaterialSlot);
    ResultObj->SetStringField(TEXT("parameter_name"), ParameterName);
    
    TArray<TSharedPtr<FJsonValue>> ColorResultArray;
    ColorResultArray.Add(MakeShared<FJsonValueNumber>(Color.R));
    ColorResultArray.Add(MakeShared<FJsonValueNumber>(Color.G));
    ColorResultArray.Add(MakeShared<FJsonValueNumber>(Color.B));
    ColorResultArray.Add(MakeShared<FJsonValueNumber>(Color.A));
    ResultObj->SetArrayField(TEXT("color"), ColorResultArray);
    
    ResultObj->SetBoolField(TEXT("success"), true);
    return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleGetAvailableMaterials(const TSharedPtr<FJsonObject>& Params)
{
    // Get parameters - make search path completely dynamic
    FString SearchPath;
    if (!Params->TryGetStringField(TEXT("search_path"), SearchPath))
    {
        // Default to empty string to search everywhere
        SearchPath = TEXT("");
    }
    
    bool bIncludeEngineMaterials = true;
    if (Params->HasField(TEXT("include_engine_materials")))
    {
        bIncludeEngineMaterials = Params->GetBoolField(TEXT("include_engine_materials"));
    }

    // Get asset registry module
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

    // Create filter for materials
    FARFilter Filter;
    Filter.ClassPaths.Add(UMaterialInterface::StaticClass()->GetClassPathName());
    Filter.ClassPaths.Add(UMaterial::StaticClass()->GetClassPathName());
    Filter.ClassPaths.Add(UMaterialInstanceConstant::StaticClass()->GetClassPathName());
    Filter.ClassPaths.Add(UMaterialInstanceDynamic::StaticClass()->GetClassPathName());
    
    // Add search paths dynamically
    if (!SearchPath.IsEmpty())
    {
        // Ensure the path starts with /
        if (!SearchPath.StartsWith(TEXT("/")))
        {
            SearchPath = TEXT("/") + SearchPath;
        }
        // Ensure the path ends with / for proper directory search
        if (!SearchPath.EndsWith(TEXT("/")))
        {
            SearchPath += TEXT("/");
        }
        Filter.PackagePaths.Add(*SearchPath);
        UE_LOG(LogTemp, Log, TEXT("Searching for materials in: %s"), *SearchPath);
    }
    else
    {
        // Search in common game content locations
        Filter.PackagePaths.Add(TEXT("/Game/"));
        UE_LOG(LogTemp, Log, TEXT("Searching for materials in all game content"));
    }
    
    if (bIncludeEngineMaterials)
    {
        Filter.PackagePaths.Add(TEXT("/Engine/"));
        UE_LOG(LogTemp, Log, TEXT("Including Engine materials in search"));
    }
    
    Filter.bRecursivePaths = true;

    // Get assets from registry
    TArray<FAssetData> AssetDataArray;
    AssetRegistry.GetAssets(Filter, AssetDataArray);
    
    UE_LOG(LogTemp, Log, TEXT("Asset registry found %d materials"), AssetDataArray.Num());

    // Also try manual search using EditorAssetLibrary for more comprehensive results
    TArray<FString> AllAssetPaths;
    if (!SearchPath.IsEmpty())
    {
        AllAssetPaths = UEditorAssetLibrary::ListAssets(SearchPath, true, false);
    }
    else
    {
        AllAssetPaths = UEditorAssetLibrary::ListAssets(TEXT("/Game/"), true, false);
    }
    
    // Filter for materials from the manual search
    for (const FString& AssetPath : AllAssetPaths)
    {
        if (AssetPath.Contains(TEXT("Material")) && !AssetPath.Contains(TEXT(".uasset")))
        {
            UObject* Asset = UEditorAssetLibrary::LoadAsset(AssetPath);
            if (Asset && Asset->IsA<UMaterialInterface>())
            {
                // Check if we already have this asset from registry search
                bool bAlreadyFound = false;
                for (const FAssetData& ExistingData : AssetDataArray)
                {
                    if (ExistingData.GetObjectPathString() == AssetPath)
                    {
                        bAlreadyFound = true;
                        break;
                    }
                }
                
                if (!bAlreadyFound)
                {
                    // Create FAssetData manually for this asset
                    FAssetData ManualAssetData(Asset);
                    AssetDataArray.Add(ManualAssetData);
                }
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("Total materials found after manual search: %d"), AssetDataArray.Num());

    // Convert to JSON
    TArray<TSharedPtr<FJsonValue>> MaterialArray;
    for (const FAssetData& AssetData : AssetDataArray)
    {
        TSharedPtr<FJsonObject> MaterialObj = MakeShared<FJsonObject>();
        MaterialObj->SetStringField(TEXT("name"), AssetData.AssetName.ToString());
        MaterialObj->SetStringField(TEXT("path"), AssetData.GetObjectPathString());
        MaterialObj->SetStringField(TEXT("package"), AssetData.PackageName.ToString());
        MaterialObj->SetStringField(TEXT("class"), AssetData.AssetClassPath.ToString());
        
        MaterialArray.Add(MakeShared<FJsonValueObject>(MaterialObj));
        
        UE_LOG(LogTemp, Verbose, TEXT("Found material: %s at %s"), *AssetData.AssetName.ToString(), *AssetData.GetObjectPathString());
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetArrayField(TEXT("materials"), MaterialArray);
    ResultObj->SetNumberField(TEXT("count"), MaterialArray.Num());
    ResultObj->SetStringField(TEXT("search_path_used"), SearchPath.IsEmpty() ? TEXT("/Game/") : SearchPath);
    
    return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleApplyMaterialToActor(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("actor_name"), ActorName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'actor_name' parameter"));
    }

    FString MaterialPath;
    if (!Params->TryGetStringField(TEXT("material_path"), MaterialPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'material_path' parameter"));
    }

    int32 MaterialSlot = 0;
    if (Params->HasField(TEXT("material_slot")))
    {
        MaterialSlot = Params->GetIntegerField(TEXT("material_slot"));
    }

    // Find the actor
    AActor* TargetActor = nullptr;
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get editor world"));
    }
    
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), AllActors);
    
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName() == ActorName)
        {
            TargetActor = Actor;
            break;
        }
    }

    if (!TargetActor)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
    }

    // Load the material
    UMaterialInterface* Material = Cast<UMaterialInterface>(UEditorAssetLibrary::LoadAsset(MaterialPath));
    if (!Material)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load material: %s"), *MaterialPath));
    }

    // Find mesh components and apply material
    TArray<UStaticMeshComponent*> MeshComponents;
    TargetActor->GetComponents<UStaticMeshComponent>(MeshComponents);
    
    bool bAppliedToAny = false;
    for (UStaticMeshComponent* MeshComp : MeshComponents)
    {
        if (MeshComp)
        {
            MeshComp->SetMaterial(MaterialSlot, Material);
            bAppliedToAny = true;
        }
    }

    if (!bAppliedToAny)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No mesh components found on actor"));
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("actor_name"), ActorName);
    ResultObj->SetStringField(TEXT("material_path"), MaterialPath);
    ResultObj->SetNumberField(TEXT("material_slot"), MaterialSlot);
    ResultObj->SetBoolField(TEXT("success"), true);
    
    return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleApplyMaterialToBlueprint(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString ComponentName;
    if (!Params->TryGetStringField(TEXT("component_name"), ComponentName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'component_name' parameter"));
    }

    FString MaterialPath;
    if (!Params->TryGetStringField(TEXT("material_path"), MaterialPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'material_path' parameter"));
    }

    int32 MaterialSlot = 0;
    if (Params->HasField(TEXT("material_slot")))
    {
        MaterialSlot = Params->GetIntegerField(TEXT("material_slot"));
    }

    // Find the blueprint
    UBlueprint* Blueprint = FEpicUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Find the component
    USCS_Node* ComponentNode = nullptr;
    for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
    {
        if (Node && Node->GetVariableName().ToString() == ComponentName)
        {
            ComponentNode = Node;
            break;
        }
    }

    if (!ComponentNode)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Component not found: %s"), *ComponentName));
    }

    UPrimitiveComponent* PrimComponent = Cast<UPrimitiveComponent>(ComponentNode->ComponentTemplate);
    if (!PrimComponent)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Component is not a primitive component"));
    }

    // Load the material
    UMaterialInterface* Material = Cast<UMaterialInterface>(UEditorAssetLibrary::LoadAsset(MaterialPath));
    if (!Material)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load material: %s"), *MaterialPath));
    }

    // Apply the material
    PrimComponent->SetMaterial(MaterialSlot, Material);

    // Mark the blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("blueprint_name"), BlueprintName);
    ResultObj->SetStringField(TEXT("component_name"), ComponentName);
    ResultObj->SetStringField(TEXT("material_path"), MaterialPath);
    ResultObj->SetNumberField(TEXT("material_slot"), MaterialSlot);
    ResultObj->SetBoolField(TEXT("success"), true);
    
    return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleGetActorMaterialInfo(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("actor_name"), ActorName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'actor_name' parameter"));
    }

    // Find the actor
    AActor* TargetActor = nullptr;
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get editor world"));
    }
    
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), AllActors);
    
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName() == ActorName)
        {
            TargetActor = Actor;
            break;
        }
    }

    if (!TargetActor)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
    }

    // Get mesh components and their materials
    TArray<UStaticMeshComponent*> MeshComponents;
    TargetActor->GetComponents<UStaticMeshComponent>(MeshComponents);
    
    TArray<TSharedPtr<FJsonValue>> MaterialSlots;
    
    for (UStaticMeshComponent* MeshComp : MeshComponents)
    {
        if (MeshComp)
        {
            for (int32 i = 0; i < MeshComp->GetNumMaterials(); i++)
            {
                TSharedPtr<FJsonObject> SlotInfo = MakeShared<FJsonObject>();
                SlotInfo->SetNumberField(TEXT("slot"), i);
                SlotInfo->SetStringField(TEXT("component"), MeshComp->GetName());
                
                UMaterialInterface* Material = MeshComp->GetMaterial(i);
                if (Material)
                {
                    SlotInfo->SetStringField(TEXT("material_name"), Material->GetName());
                    SlotInfo->SetStringField(TEXT("material_path"), Material->GetPathName());
                    SlotInfo->SetStringField(TEXT("material_class"), Material->GetClass()->GetName());
                }
                else
                {
                    SlotInfo->SetStringField(TEXT("material_name"), TEXT("None"));
                    SlotInfo->SetStringField(TEXT("material_path"), TEXT(""));
                    SlotInfo->SetStringField(TEXT("material_class"), TEXT(""));
                }
                
                MaterialSlots.Add(MakeShared<FJsonValueObject>(SlotInfo));
            }
        }
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("actor_name"), ActorName);
    ResultObj->SetArrayField(TEXT("material_slots"), MaterialSlots);
    ResultObj->SetNumberField(TEXT("total_slots"), MaterialSlots.Num());
    
    return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleGetBlueprintMaterialInfo(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString ComponentName;
    if (!Params->TryGetStringField(TEXT("component_name"), ComponentName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'component_name' parameter"));
    }

    // Find the blueprint
    UBlueprint* Blueprint = FEpicUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Find the component
    USCS_Node* ComponentNode = nullptr;
    for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
    {
        if (Node && Node->GetVariableName().ToString() == ComponentName)
        {
            ComponentNode = Node;
            break;
        }
    }

    if (!ComponentNode)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Component not found: %s"), *ComponentName));
    }

    UStaticMeshComponent* MeshComponent = Cast<UStaticMeshComponent>(ComponentNode->ComponentTemplate);
    if (!MeshComponent)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Component is not a static mesh component"));
    }

    // Get material slot information
    TArray<TSharedPtr<FJsonValue>> MaterialSlots;
    int32 NumMaterials = 0;
    
    // Check if we have a static mesh assigned
    UStaticMesh* StaticMesh = MeshComponent->GetStaticMesh();
    if (StaticMesh)
    {
        NumMaterials = StaticMesh->GetNumSections(0); // Get number of material slots for LOD 0
        
        for (int32 i = 0; i < NumMaterials; i++)
        {
            TSharedPtr<FJsonObject> SlotInfo = MakeShared<FJsonObject>();
            SlotInfo->SetNumberField(TEXT("slot"), i);
            SlotInfo->SetStringField(TEXT("component"), ComponentName);
            
            UMaterialInterface* Material = MeshComponent->GetMaterial(i);
            if (Material)
            {
                SlotInfo->SetStringField(TEXT("material_name"), Material->GetName());
                SlotInfo->SetStringField(TEXT("material_path"), Material->GetPathName());
                SlotInfo->SetStringField(TEXT("material_class"), Material->GetClass()->GetName());
            }
            else
            {
                SlotInfo->SetStringField(TEXT("material_name"), TEXT("None"));
                SlotInfo->SetStringField(TEXT("material_path"), TEXT(""));
                SlotInfo->SetStringField(TEXT("material_class"), TEXT(""));
            }
            
            MaterialSlots.Add(MakeShared<FJsonValueObject>(SlotInfo));
        }
    }
    else
    {
        // If no static mesh is assigned, we can't determine material slots
        UE_LOG(LogTemp, Warning, TEXT("No static mesh assigned to component %s in blueprint %s"), *ComponentName, *BlueprintName);
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("blueprint_name"), BlueprintName);
    ResultObj->SetStringField(TEXT("component_name"), ComponentName);
    ResultObj->SetArrayField(TEXT("material_slots"), MaterialSlots);
    ResultObj->SetNumberField(TEXT("total_slots"), MaterialSlots.Num());
    ResultObj->SetBoolField(TEXT("has_static_mesh"), StaticMesh != nullptr);
    
    return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleReadBlueprintContent(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintPath;
    if (!Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_path' parameter"));
    }

    // Get optional parameters
    bool bIncludeEventGraph = true;
    bool bIncludeFunctions = true;
    bool bIncludeVariables = true;
    bool bIncludeComponents = true;
    bool bIncludeInterfaces = true;

    Params->TryGetBoolField(TEXT("include_event_graph"), bIncludeEventGraph);
    Params->TryGetBoolField(TEXT("include_functions"), bIncludeFunctions);
    Params->TryGetBoolField(TEXT("include_variables"), bIncludeVariables);
    Params->TryGetBoolField(TEXT("include_components"), bIncludeComponents);
    Params->TryGetBoolField(TEXT("include_interfaces"), bIncludeInterfaces);

    // Load the blueprint
    UBlueprint* Blueprint = Cast<UBlueprint>(UEditorAssetLibrary::LoadAsset(BlueprintPath));
    if (!Blueprint)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load blueprint: %s"), *BlueprintPath));
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("blueprint_path"), BlueprintPath);
    ResultObj->SetStringField(TEXT("blueprint_name"), Blueprint->GetName());
    ResultObj->SetStringField(TEXT("parent_class"), Blueprint->ParentClass ? Blueprint->ParentClass->GetName() : TEXT("None"));

    // Include variables if requested
    if (bIncludeVariables)
    {
        TArray<TSharedPtr<FJsonValue>> VariableArray;
        for (const FBPVariableDescription& Variable : Blueprint->NewVariables)
        {
            TSharedPtr<FJsonObject> VarObj = MakeShared<FJsonObject>();
            VarObj->SetStringField(TEXT("name"), Variable.VarName.ToString());
            VarObj->SetStringField(TEXT("type"), Variable.VarType.PinCategory.ToString());
            VarObj->SetStringField(TEXT("default_value"), Variable.DefaultValue);
            VarObj->SetBoolField(TEXT("is_editable"), (Variable.PropertyFlags & CPF_Edit) != 0);
            VariableArray.Add(MakeShared<FJsonValueObject>(VarObj));
        }
        ResultObj->SetArrayField(TEXT("variables"), VariableArray);
    }

    // Include functions if requested
    if (bIncludeFunctions)
    {
        TArray<TSharedPtr<FJsonValue>> FunctionArray;
        for (UEdGraph* Graph : Blueprint->FunctionGraphs)
        {
            if (Graph)
            {
                TSharedPtr<FJsonObject> FuncObj = MakeShared<FJsonObject>();
                FuncObj->SetStringField(TEXT("name"), Graph->GetName());
                FuncObj->SetStringField(TEXT("graph_type"), TEXT("Function"));
                
                // Count nodes in function
                int32 NodeCount = Graph->Nodes.Num();
                FuncObj->SetNumberField(TEXT("node_count"), NodeCount);
                
                FunctionArray.Add(MakeShared<FJsonValueObject>(FuncObj));
            }
        }
        ResultObj->SetArrayField(TEXT("functions"), FunctionArray);
    }

    // Include event graph if requested
    if (bIncludeEventGraph)
    {
        TSharedPtr<FJsonObject> EventGraphObj = MakeShared<FJsonObject>();
        
        // Find the main event graph
        for (UEdGraph* Graph : Blueprint->UbergraphPages)
        {
            if (Graph && Graph->GetName() == TEXT("EventGraph"))
            {
                EventGraphObj->SetStringField(TEXT("name"), Graph->GetName());
                EventGraphObj->SetNumberField(TEXT("node_count"), Graph->Nodes.Num());
                
                // Get basic node information
                TArray<TSharedPtr<FJsonValue>> NodeArray;
                for (UEdGraphNode* Node : Graph->Nodes)
                {
                    if (Node)
                    {
                        TSharedPtr<FJsonObject> NodeObj = MakeShared<FJsonObject>();
                        NodeObj->SetStringField(TEXT("name"), Node->GetName());
                        NodeObj->SetStringField(TEXT("class"), Node->GetClass()->GetName());
                        NodeObj->SetStringField(TEXT("title"), Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString());
                        NodeArray.Add(MakeShared<FJsonValueObject>(NodeObj));
                    }
                }
                EventGraphObj->SetArrayField(TEXT("nodes"), NodeArray);
                break;
            }
        }
        
        ResultObj->SetObjectField(TEXT("event_graph"), EventGraphObj);
    }

    // Include components if requested
    if (bIncludeComponents)
    {
        TArray<TSharedPtr<FJsonValue>> ComponentArray;
        if (Blueprint->SimpleConstructionScript)
        {
            USimpleConstructionScript* SCS = Blueprint->SimpleConstructionScript;
            TMap<USCS_Node*, USCS_Node*> ChildToParent;
            for (USCS_Node* Node : SCS->GetAllNodes())
            {
                if (!Node)
                {
                    continue;
                }
                for (USCS_Node* Child : Node->GetChildNodes())
                {
                    if (Child)
                    {
                        ChildToParent.Add(Child, Node);
                    }
                }
            }

            for (USCS_Node* Node : SCS->GetAllNodes())
            {
                if (Node && Node->ComponentTemplate)
                {
                    TSharedPtr<FJsonObject> CompObj = MakeShared<FJsonObject>();
                    CompObj->SetStringField(TEXT("name"), Node->GetVariableName().ToString());
                    CompObj->SetStringField(TEXT("class"), Node->ComponentTemplate->GetClass()->GetName());
                    CompObj->SetBoolField(TEXT("is_root"), Node->IsRootNode());
                    CompObj->SetBoolField(TEXT("is_default_scene_root"), Node == SCS->GetDefaultSceneRootNode());

                    if (USCS_Node** ParentNode = ChildToParent.Find(Node))
                    {
                        CompObj->SetStringField(TEXT("parent"), (*ParentNode)->GetVariableName().ToString());
                    }
                    else if (Node->IsRootNode())
                    {
                        CompObj->SetField(TEXT("parent"), MakeShared<FJsonValueNull>());
                    }
                    else
                    {
                        CompObj->SetField(TEXT("parent"), MakeShared<FJsonValueNull>());
                    }

                    if (USceneComponent* Scene = Cast<USceneComponent>(Node->ComponentTemplate))
                    {
                        const FVector Loc = Scene->GetRelativeLocation();
                        const FRotator Rot = Scene->GetRelativeRotation();
                        const FVector Scale = Scene->GetRelativeScale3D();

                        TSharedPtr<FJsonObject> Rel = MakeShared<FJsonObject>();
                        TArray<TSharedPtr<FJsonValue>> LocArr;
                        LocArr.Add(MakeShared<FJsonValueNumber>(Loc.X));
                        LocArr.Add(MakeShared<FJsonValueNumber>(Loc.Y));
                        LocArr.Add(MakeShared<FJsonValueNumber>(Loc.Z));
                        Rel->SetArrayField(TEXT("location"), LocArr);

                        TArray<TSharedPtr<FJsonValue>> RotArr;
                        RotArr.Add(MakeShared<FJsonValueNumber>(Rot.Roll));
                        RotArr.Add(MakeShared<FJsonValueNumber>(Rot.Pitch));
                        RotArr.Add(MakeShared<FJsonValueNumber>(Rot.Yaw));
                        Rel->SetArrayField(TEXT("rotation"), RotArr);

                        TArray<TSharedPtr<FJsonValue>> ScaleArr;
                        ScaleArr.Add(MakeShared<FJsonValueNumber>(Scale.X));
                        ScaleArr.Add(MakeShared<FJsonValueNumber>(Scale.Y));
                        ScaleArr.Add(MakeShared<FJsonValueNumber>(Scale.Z));
                        Rel->SetArrayField(TEXT("scale"), ScaleArr);

                        CompObj->SetObjectField(TEXT("relative"), Rel);
                        CompObj->SetBoolField(TEXT("is_scene"), true);
                    }
                    else
                    {
                        CompObj->SetBoolField(TEXT("is_scene"), false);
                    }

                    ComponentArray.Add(MakeShared<FJsonValueObject>(CompObj));
                }
            }
        }
        ResultObj->SetArrayField(TEXT("components"), ComponentArray);
    }

    // Include interfaces if requested
    if (bIncludeInterfaces)
    {
        TArray<TSharedPtr<FJsonValue>> InterfaceArray;
        for (const FBPInterfaceDescription& Interface : Blueprint->ImplementedInterfaces)
        {
            TSharedPtr<FJsonObject> InterfaceObj = MakeShared<FJsonObject>();
            InterfaceObj->SetStringField(TEXT("name"), Interface.Interface ? Interface.Interface->GetName() : TEXT("Unknown"));
            InterfaceArray.Add(MakeShared<FJsonValueObject>(InterfaceObj));
        }
        ResultObj->SetArrayField(TEXT("interfaces"), InterfaceArray);
    }

    ResultObj->SetBoolField(TEXT("success"), true);
    return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleAnalyzeBlueprintGraph(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintPath;
    if (!Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_path' parameter"));
    }

    FString GraphName = TEXT("EventGraph");
    Params->TryGetStringField(TEXT("graph_name"), GraphName);

    // Get optional parameters
    bool bIncludeNodeDetails = true;
    bool bIncludePinConnections = true;
    bool bTraceExecutionFlow = true;

    Params->TryGetBoolField(TEXT("include_node_details"), bIncludeNodeDetails);
    Params->TryGetBoolField(TEXT("include_pin_connections"), bIncludePinConnections);
    Params->TryGetBoolField(TEXT("trace_execution_flow"), bTraceExecutionFlow);

    // Load the blueprint
    UBlueprint* Blueprint = Cast<UBlueprint>(UEditorAssetLibrary::LoadAsset(BlueprintPath));
    if (!Blueprint)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load blueprint: %s"), *BlueprintPath));
    }

    // Find the specified graph
    UEdGraph* TargetGraph = nullptr;
    
    // Check event graphs first
    for (UEdGraph* Graph : Blueprint->UbergraphPages)
    {
        if (Graph && Graph->GetName() == GraphName)
        {
            TargetGraph = Graph;
            break;
        }
    }
    
    // Check function graphs if not found
    if (!TargetGraph)
    {
        for (UEdGraph* Graph : Blueprint->FunctionGraphs)
        {
            if (Graph && Graph->GetName() == GraphName)
            {
                TargetGraph = Graph;
                break;
            }
        }
    }

    if (!TargetGraph)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Graph not found: %s"), *GraphName));
    }

    TSharedPtr<FJsonObject> GraphData = MakeShared<FJsonObject>();
    GraphData->SetStringField(TEXT("graph_name"), TargetGraph->GetName());
    GraphData->SetStringField(TEXT("graph_type"), TargetGraph->GetClass()->GetName());

    // Analyze nodes
    TArray<TSharedPtr<FJsonValue>> NodeArray;
    TArray<TSharedPtr<FJsonValue>> ConnectionArray;

    for (UEdGraphNode* Node : TargetGraph->Nodes)
    {
        if (Node)
        {
            TSharedPtr<FJsonObject> NodeObj = MakeShared<FJsonObject>();
            NodeObj->SetStringField(TEXT("name"), Node->GetName());
            NodeObj->SetStringField(TEXT("class"), Node->GetClass()->GetName());
            NodeObj->SetStringField(TEXT("title"), Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString());

            if (bIncludeNodeDetails)
            {
                NodeObj->SetNumberField(TEXT("pos_x"), Node->NodePosX);
                NodeObj->SetNumberField(TEXT("pos_y"), Node->NodePosY);
                NodeObj->SetBoolField(TEXT("can_rename"), Node->bCanRenameNode);
            }

            // Include pin information if requested
            if (bIncludePinConnections)
            {
                TArray<TSharedPtr<FJsonValue>> PinArray;
                for (UEdGraphPin* Pin : Node->Pins)
                {
                    if (Pin)
                    {
                        TSharedPtr<FJsonObject> PinObj = MakeShared<FJsonObject>();
                        PinObj->SetStringField(TEXT("name"), Pin->PinName.ToString());
                        PinObj->SetStringField(TEXT("type"), Pin->PinType.PinCategory.ToString());
                        PinObj->SetStringField(TEXT("direction"), Pin->Direction == EGPD_Input ? TEXT("Input") : TEXT("Output"));
                        PinObj->SetNumberField(TEXT("connections"), Pin->LinkedTo.Num());
                        
                        // Record connections for this pin
                        for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
                        {
                            if (LinkedPin && LinkedPin->GetOwningNode())
                            {
                                TSharedPtr<FJsonObject> ConnObj = MakeShared<FJsonObject>();
                                ConnObj->SetStringField(TEXT("from_node"), Pin->GetOwningNode()->GetName());
                                ConnObj->SetStringField(TEXT("from_pin"), Pin->PinName.ToString());
                                ConnObj->SetStringField(TEXT("to_node"), LinkedPin->GetOwningNode()->GetName());
                                ConnObj->SetStringField(TEXT("to_pin"), LinkedPin->PinName.ToString());
                                ConnectionArray.Add(MakeShared<FJsonValueObject>(ConnObj));
                            }
                        }
                        
                        PinArray.Add(MakeShared<FJsonValueObject>(PinObj));
                    }
                }
                NodeObj->SetArrayField(TEXT("pins"), PinArray);
            }

            NodeArray.Add(MakeShared<FJsonValueObject>(NodeObj));
        }
    }

    GraphData->SetArrayField(TEXT("nodes"), NodeArray);
    GraphData->SetArrayField(TEXT("connections"), ConnectionArray);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("blueprint_path"), BlueprintPath);
    ResultObj->SetObjectField(TEXT("graph_data"), GraphData);
    ResultObj->SetBoolField(TEXT("success"), true);

    return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleGetBlueprintVariableDetails(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintPath;
    if (!Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_path' parameter"));
    }

    FString VariableName;
    bool bSpecificVariable = Params->TryGetStringField(TEXT("variable_name"), VariableName);

    // Load the blueprint
    UBlueprint* Blueprint = Cast<UBlueprint>(UEditorAssetLibrary::LoadAsset(BlueprintPath));
    if (!Blueprint)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load blueprint: %s"), *BlueprintPath));
    }

    TArray<TSharedPtr<FJsonValue>> VariableArray;

    for (const FBPVariableDescription& Variable : Blueprint->NewVariables)
    {
        // If looking for specific variable, skip others
        if (bSpecificVariable && Variable.VarName.ToString() != VariableName)
        {
            continue;
        }

        TSharedPtr<FJsonObject> VarObj = MakeShared<FJsonObject>();
        VarObj->SetStringField(TEXT("name"), Variable.VarName.ToString());
        VarObj->SetStringField(TEXT("type"), Variable.VarType.PinCategory.ToString());
        VarObj->SetStringField(TEXT("sub_category"), Variable.VarType.PinSubCategory.ToString());
        VarObj->SetStringField(TEXT("default_value"), Variable.DefaultValue);
        VarObj->SetStringField(TEXT("friendly_name"), Variable.FriendlyName.IsEmpty() ? Variable.VarName.ToString() : Variable.FriendlyName);
        
        // Get tooltip from metadata (VarTooltip doesn't exist in UE 5.5)
        FString TooltipValue;
        if (Variable.HasMetaData(FBlueprintMetadata::MD_Tooltip))
        {
            TooltipValue = Variable.GetMetaData(FBlueprintMetadata::MD_Tooltip);
        }
        VarObj->SetStringField(TEXT("tooltip"), TooltipValue);
        
        VarObj->SetStringField(TEXT("category"), Variable.Category.ToString());

        // Property flags
        VarObj->SetBoolField(TEXT("is_editable"), (Variable.PropertyFlags & CPF_Edit) != 0);
        VarObj->SetBoolField(TEXT("is_blueprint_visible"), (Variable.PropertyFlags & CPF_BlueprintVisible) != 0);
        VarObj->SetBoolField(TEXT("is_editable_in_instance"), (Variable.PropertyFlags & CPF_DisableEditOnInstance) == 0);
        VarObj->SetBoolField(TEXT("is_config"), (Variable.PropertyFlags & CPF_Config) != 0);

        // Replication
        VarObj->SetNumberField(TEXT("replication"), (int32)Variable.ReplicationCondition);

        VariableArray.Add(MakeShared<FJsonValueObject>(VarObj));
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("blueprint_path"), BlueprintPath);
    
    if (bSpecificVariable)
    {
        ResultObj->SetStringField(TEXT("variable_name"), VariableName);
        if (VariableArray.Num() > 0)
        {
            ResultObj->SetObjectField(TEXT("variable"), VariableArray[0]->AsObject());
        }
        else
        {
            return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Variable not found: %s"), *VariableName));
        }
    }
    else
    {
        ResultObj->SetArrayField(TEXT("variables"), VariableArray);
        ResultObj->SetNumberField(TEXT("variable_count"), VariableArray.Num());
    }

    ResultObj->SetBoolField(TEXT("success"), true);
    return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleGetBlueprintFunctionDetails(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintPath;
    if (!Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_path' parameter"));
    }

    FString FunctionName;
    bool bSpecificFunction = Params->TryGetStringField(TEXT("function_name"), FunctionName);

    bool bIncludeGraph = true;
    Params->TryGetBoolField(TEXT("include_graph"), bIncludeGraph);

    // Load the blueprint
    UBlueprint* Blueprint = Cast<UBlueprint>(UEditorAssetLibrary::LoadAsset(BlueprintPath));
    if (!Blueprint)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load blueprint: %s"), *BlueprintPath));
    }

    TArray<TSharedPtr<FJsonValue>> FunctionArray;

    for (UEdGraph* Graph : Blueprint->FunctionGraphs)
    {
        if (!Graph) continue;

        // If looking for specific function, skip others
        if (bSpecificFunction && Graph->GetName() != FunctionName)
        {
            continue;
        }

        TSharedPtr<FJsonObject> FuncObj = MakeShared<FJsonObject>();
        FuncObj->SetStringField(TEXT("name"), Graph->GetName());
        FuncObj->SetStringField(TEXT("graph_type"), TEXT("Function"));

        // Get function signature from graph
        TArray<TSharedPtr<FJsonValue>> InputPins;
        TArray<TSharedPtr<FJsonValue>> OutputPins;

        // Find function entry and result nodes
        for (UEdGraphNode* Node : Graph->Nodes)
        {
            if (Node)
            {
                if (Node->GetClass()->GetName().Contains(TEXT("FunctionEntry")))
                {
                    // Process input parameters
                    for (UEdGraphPin* Pin : Node->Pins)
                    {
                        if (Pin && Pin->Direction == EGPD_Output && Pin->PinName != TEXT("then"))
                        {
                            TSharedPtr<FJsonObject> PinObj = MakeShared<FJsonObject>();
                            PinObj->SetStringField(TEXT("name"), Pin->PinName.ToString());
                            PinObj->SetStringField(TEXT("type"), Pin->PinType.PinCategory.ToString());
                            InputPins.Add(MakeShared<FJsonValueObject>(PinObj));
                        }
                    }
                }
                else if (Node->GetClass()->GetName().Contains(TEXT("FunctionResult")))
                {
                    // Process output parameters
                    for (UEdGraphPin* Pin : Node->Pins)
                    {
                        if (Pin && Pin->Direction == EGPD_Input && Pin->PinName != TEXT("exec"))
                        {
                            TSharedPtr<FJsonObject> PinObj = MakeShared<FJsonObject>();
                            PinObj->SetStringField(TEXT("name"), Pin->PinName.ToString());
                            PinObj->SetStringField(TEXT("type"), Pin->PinType.PinCategory.ToString());
                            OutputPins.Add(MakeShared<FJsonValueObject>(PinObj));
                        }
                    }
                }
            }
        }

        FuncObj->SetArrayField(TEXT("input_parameters"), InputPins);
        FuncObj->SetArrayField(TEXT("output_parameters"), OutputPins);
        FuncObj->SetNumberField(TEXT("node_count"), Graph->Nodes.Num());

        // Include graph details if requested
        if (bIncludeGraph)
        {
            TArray<TSharedPtr<FJsonValue>> NodeArray;
            for (UEdGraphNode* Node : Graph->Nodes)
            {
                if (Node)
                {
                    TSharedPtr<FJsonObject> NodeObj = MakeShared<FJsonObject>();
                    NodeObj->SetStringField(TEXT("name"), Node->GetName());
                    NodeObj->SetStringField(TEXT("class"), Node->GetClass()->GetName());
                    NodeObj->SetStringField(TEXT("title"), Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString());
                    NodeArray.Add(MakeShared<FJsonValueObject>(NodeObj));
                }
            }
            FuncObj->SetArrayField(TEXT("graph_nodes"), NodeArray);
        }

        FunctionArray.Add(MakeShared<FJsonValueObject>(FuncObj));
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("blueprint_path"), BlueprintPath);
    
    if (bSpecificFunction)
    {
        ResultObj->SetStringField(TEXT("function_name"), FunctionName);
        if (FunctionArray.Num() > 0)
        {
            ResultObj->SetObjectField(TEXT("function"), FunctionArray[0]->AsObject());
        }
        else
        {
            return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Function not found: %s"), *FunctionName));
        }
    }
    else
    {
        ResultObj->SetArrayField(TEXT("functions"), FunctionArray);
        ResultObj->SetNumberField(TEXT("function_count"), FunctionArray.Num());
    }

    ResultObj->SetBoolField(TEXT("success"), true);
    return ResultObj;
}

namespace
{
	USCS_Node* FindSCSNodeByName(USimpleConstructionScript* SCS, const FName& Name)
	{
		if (!SCS)
		{
			return nullptr;
		}
		for (USCS_Node* Node : SCS->GetAllNodes())
		{
			if (Node && Node->GetVariableName() == Name)
			{
				return Node;
			}
		}
		return nullptr;
	}

	USCS_Node* FindParentSCSNode(USimpleConstructionScript* SCS, USCS_Node* Child)
	{
		if (!SCS || !Child)
		{
			return nullptr;
		}
		for (USCS_Node* Node : SCS->GetAllNodes())
		{
			if (!Node)
			{
				continue;
			}
			if (Node->GetChildNodes().Contains(Child))
			{
				return Node;
			}
		}
		return nullptr;
	}

	TSharedPtr<FJsonObject> BuildComponentHierarchyJson(UBlueprint* Blueprint)
	{
		TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
		TArray<TSharedPtr<FJsonValue>> ComponentArray;

		if (!Blueprint || !Blueprint->SimpleConstructionScript)
		{
			ResultObj->SetArrayField(TEXT("components"), ComponentArray);
			return ResultObj;
		}

		USimpleConstructionScript* SCS = Blueprint->SimpleConstructionScript;
		TMap<USCS_Node*, USCS_Node*> ChildToParent;
		for (USCS_Node* Node : SCS->GetAllNodes())
		{
			if (!Node)
			{
				continue;
			}
			for (USCS_Node* Child : Node->GetChildNodes())
			{
				if (Child)
				{
					ChildToParent.Add(Child, Node);
				}
			}
		}

		for (USCS_Node* Node : SCS->GetAllNodes())
		{
			if (!Node || !Node->ComponentTemplate)
			{
				continue;
			}

			TSharedPtr<FJsonObject> CompObj = MakeShared<FJsonObject>();
			CompObj->SetStringField(TEXT("name"), Node->GetVariableName().ToString());
			CompObj->SetStringField(TEXT("class"), Node->ComponentTemplate->GetClass()->GetName());
			CompObj->SetBoolField(TEXT("is_root"), Node->IsRootNode());
			CompObj->SetBoolField(TEXT("is_default_scene_root"), Node == SCS->GetDefaultSceneRootNode());

			if (USCS_Node** ParentNode = ChildToParent.Find(Node))
			{
				CompObj->SetStringField(TEXT("parent"), (*ParentNode)->GetVariableName().ToString());
			}
			else
			{
				CompObj->SetField(TEXT("parent"), MakeShared<FJsonValueNull>());
			}

			if (USceneComponent* Scene = Cast<USceneComponent>(Node->ComponentTemplate))
			{
				const FVector Loc = Scene->GetRelativeLocation();
				const FRotator Rot = Scene->GetRelativeRotation();
				const FVector Scale = Scene->GetRelativeScale3D();

				TSharedPtr<FJsonObject> Rel = MakeShared<FJsonObject>();
				TArray<TSharedPtr<FJsonValue>> LocArr{ MakeShared<FJsonValueNumber>(Loc.X), MakeShared<FJsonValueNumber>(Loc.Y), MakeShared<FJsonValueNumber>(Loc.Z) };
				TArray<TSharedPtr<FJsonValue>> RotArr{ MakeShared<FJsonValueNumber>(Rot.Roll), MakeShared<FJsonValueNumber>(Rot.Pitch), MakeShared<FJsonValueNumber>(Rot.Yaw) };
				TArray<TSharedPtr<FJsonValue>> ScaleArr{ MakeShared<FJsonValueNumber>(Scale.X), MakeShared<FJsonValueNumber>(Scale.Y), MakeShared<FJsonValueNumber>(Scale.Z) };
				Rel->SetArrayField(TEXT("location"), LocArr);
				Rel->SetArrayField(TEXT("rotation"), RotArr);
				Rel->SetArrayField(TEXT("scale"), ScaleArr);
				CompObj->SetObjectField(TEXT("relative"), Rel);
				CompObj->SetBoolField(TEXT("is_scene"), true);
			}
			else
			{
				CompObj->SetBoolField(TEXT("is_scene"), false);
			}

			ComponentArray.Add(MakeShared<FJsonValueObject>(CompObj));
		}

		ResultObj->SetArrayField(TEXT("components"), ComponentArray);
		return ResultObj;
	}
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleGetBlueprintComponentHierarchy(const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintPath;
	if (!Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_path' parameter"));
	}

	UBlueprint* Blueprint = Cast<UBlueprint>(UEditorAssetLibrary::LoadAsset(BlueprintPath));
	if (!Blueprint)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load blueprint: %s"), *BlueprintPath));
	}

	TSharedPtr<FJsonObject> ResultObj = BuildComponentHierarchyJson(Blueprint);
	ResultObj->SetStringField(TEXT("blueprint_path"), BlueprintPath);
	ResultObj->SetBoolField(TEXT("success"), true);
	return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleFixBlueprintScaledRoot(const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintPath;
	if (!Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_path' parameter"));
	}

	FString ScaledComponentName = TEXT("Body");
	Params->TryGetStringField(TEXT("scaled_component"), ScaledComponentName);

	FString RootName = TEXT("Root");
	Params->TryGetStringField(TEXT("root_name"), RootName);

	UBlueprint* Blueprint = Cast<UBlueprint>(UEditorAssetLibrary::LoadAsset(BlueprintPath));
	if (!Blueprint || !Blueprint->SimpleConstructionScript)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load blueprint SCS: %s"), *BlueprintPath));
	}

	USimpleConstructionScript* SCS = Blueprint->SimpleConstructionScript;
	SCS->Modify();
	Blueprint->Modify();

	TArray<FString> Actions;
	USCS_Node* BodyNode = FindSCSNodeByName(SCS, FName(*ScaledComponentName));
	if (!BodyNode)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Scaled component not found: %s"), *ScaledComponentName));
	}

	// Find or create an unscaled SceneComponent that will become the true scene root.
	// Note: USCS_Node::SetParent only writes metadata — ChildNodes/RootNodes are the real tree.
	USCS_Node* RootNode = FindSCSNodeByName(SCS, FName(*RootName));
	if (!RootNode)
	{
		RootNode = FindSCSNodeByName(SCS, TEXT("DefaultSceneRoot"));
		if (RootNode)
		{
			RootNode->SetVariableName(FName(*RootName));
			Actions.Add(TEXT("Renamed DefaultSceneRoot -> Root"));
		}
	}

	const bool bCreatedRoot = (RootNode == nullptr);
	if (!RootNode)
	{
		RootNode = SCS->CreateNode(USceneComponent::StaticClass(), FName(*RootName));
		if (!RootNode)
		{
			return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create Root SceneComponent"));
		}
		Actions.Add(TEXT("Created Root SceneComponent"));
	}

	if (USceneComponent* RootScene = Cast<USceneComponent>(RootNode->ComponentTemplate))
	{
		RootScene->Modify();
		RootScene->SetRelativeLocation(FVector::ZeroVector);
		RootScene->SetRelativeRotation(FRotator::ZeroRotator);
		RootScene->SetRelativeScale3D(FVector::OneVector);
	}

	// Capture Body's direct children + relative transform before mutating the tree.
	USceneComponent* BodyScene = Cast<USceneComponent>(BodyNode->ComponentTemplate);
	const FTransform BodyRel = BodyScene ? BodyScene->GetRelativeTransform() : FTransform::Identity;

	TArray<USCS_Node*> DirectChildren;
	for (USCS_Node* Child : BodyNode->GetChildNodes())
	{
		if (Child && Child != RootNode)
		{
			DirectChildren.Add(Child);
		}
	}

	// If a previous attempt nested Root under Body, detach it and drop from AllNodes
	// so AddNode can re-add it cleanly as the scene root.
	if (USCS_Node* RootParent = FindParentSCSNode(SCS, RootNode))
	{
		RootParent->RemoveChildNode(RootNode, /*bRemoveFromAllNodes=*/true);
		Actions.Add(FString::Printf(TEXT("Detached Root from %s"), *RootParent->GetVariableName().ToString()));
	}

	// Mirror USubobjectDataSubsystem::MakeNewSceneRoot for SCS:
	// 1) Remove scaled Body from RootNodes (without validation)
	// 2) Add Root as the scene root
	// 3) Attach Body under Root via AddChildNode
	if (BodyNode->IsRootNode())
	{
		SCS->RemoveNode(BodyNode, /*bValidateSceneRootNodes=*/false);
		Actions.Add(FString::Printf(TEXT("Removed %s from SCS root list"), *ScaledComponentName));
	}
	else if (USCS_Node* BodyParent = FindParentSCSNode(SCS, BodyNode))
	{
		if (BodyParent != RootNode)
		{
			BodyParent->RemoveChildNode(BodyNode, /*bRemoveFromAllNodes=*/false);
			Actions.Add(FString::Printf(TEXT("Detached %s from %s"), *ScaledComponentName, *BodyParent->GetVariableName().ToString()));
		}
	}

	if (!RootNode->IsRootNode())
	{
		// AddNode puts Root into RootNodes and AllNodes, then validates.
		// If Root was already in AllNodes (e.g. after detach), AddNode still works if not in RootNodes.
		if (!SCS->GetRootNodes().Contains(RootNode))
		{
			SCS->AddNode(RootNode);
			Actions.Add(bCreatedRoot ? TEXT("Added Root as scene root") : TEXT("Promoted Root to scene root"));
		}
	}

	if (FindParentSCSNode(SCS, BodyNode) != RootNode)
	{
		RootNode->AddChildNode(BodyNode, /*bAddToAllNodes=*/true);
		BodyNode->SetParent(RootNode);
		Actions.Add(FString::Printf(TEXT("Attached %s under Root"), *ScaledComponentName));
	}

	// Bake Body's transform into each former child, then attach them under Root.
	for (USCS_Node* Child : DirectChildren)
	{
		if (!Child || Child == RootNode || Child == BodyNode)
		{
			continue;
		}

		if (USceneComponent* ChildScene = Cast<USceneComponent>(Child->ComponentTemplate))
		{
			ChildScene->Modify();
			const FTransform NewRel = BodyRel * ChildScene->GetRelativeTransform();
			ChildScene->SetRelativeLocation(NewRel.GetLocation());
			ChildScene->SetRelativeRotation(NewRel.Rotator());
			ChildScene->SetRelativeScale3D(NewRel.GetScale3D());
		}

		if (FindParentSCSNode(SCS, Child) == BodyNode)
		{
			BodyNode->RemoveChildNode(Child, /*bRemoveFromAllNodes=*/false);
		}
		else if (USCS_Node* ExistingParent = FindParentSCSNode(SCS, Child))
		{
			if (ExistingParent != RootNode)
			{
				ExistingParent->RemoveChildNode(Child, /*bRemoveFromAllNodes=*/false);
			}
		}

		if (FindParentSCSNode(SCS, Child) != RootNode)
		{
			RootNode->AddChildNode(Child, /*bAddToAllNodes=*/true);
			Child->SetParent(RootNode);
		}

		Actions.Add(FString::Printf(TEXT("Moved %s from %s -> Root (transform baked)"), *Child->GetVariableName().ToString(), *ScaledComponentName));
	}

	SCS->ValidateSceneRootNodes();
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	FKismetEditorUtilities::CompileBlueprint(Blueprint);
	UEditorAssetLibrary::SaveLoadedAsset(Blueprint);
	Actions.Add(TEXT("Compiled and saved"));

	TSharedPtr<FJsonObject> ResultObj = BuildComponentHierarchyJson(Blueprint);
	ResultObj->SetStringField(TEXT("blueprint_path"), BlueprintPath);
	ResultObj->SetBoolField(TEXT("success"), true);

	TArray<TSharedPtr<FJsonValue>> ActionArr;
	for (const FString& A : Actions)
	{
		ActionArr.Add(MakeShared<FJsonValueString>(A));
	}
	ResultObj->SetArrayField(TEXT("actions"), ActionArr);
	return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleCorrectBlueprintRelativeBake(const TSharedPtr<FJsonObject>& Params)
{
	// Undoes one extra ParentRel bake on children of Root (except ScaledComponent).
	// Used when a prior failed reparent baked transforms while nodes were still parented
	// under the scaled component, then a successful reparent baked again.
	FString BlueprintPath;
	if (!Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_path' parameter"));
	}

	FString ScaledComponentName = TEXT("Body");
	Params->TryGetStringField(TEXT("scaled_component"), ScaledComponentName);

	FString RootName = TEXT("Root");
	Params->TryGetStringField(TEXT("root_name"), RootName);

	bool bRebuildAllNodesOnly = false;
	Params->TryGetBoolField(TEXT("rebuild_all_nodes_only"), bRebuildAllNodesOnly);

	UBlueprint* Blueprint = Cast<UBlueprint>(UEditorAssetLibrary::LoadAsset(BlueprintPath));
	if (!Blueprint || !Blueprint->SimpleConstructionScript)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to load blueprint"));
	}

	USimpleConstructionScript* SCS = Blueprint->SimpleConstructionScript;
	USCS_Node* RootNode = FindSCSNodeByName(SCS, FName(*RootName));
	USCS_Node* BodyNode = FindSCSNodeByName(SCS, FName(*ScaledComponentName));
	if (!RootNode || !BodyNode)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Root or Body node missing"));
	}

	USceneComponent* BodyScene = Cast<USceneComponent>(BodyNode->ComponentTemplate);
	const FTransform BodyRel = BodyScene ? BodyScene->GetRelativeTransform() : FTransform::Identity;
	const FTransform InvBodyRel = BodyRel.Inverse();

	TArray<FString> Actions;
	SCS->Modify();
	Blueprint->Modify();

	// Rebuild AllNodes from the live RootNodes tree (dedupes after RemoveNode/AddNode cycles).
	{
		TArray<USCS_Node*> Rebuilt;
		TSet<USCS_Node*> Seen;

		TFunction<void(USCS_Node*)> Gather = [&](USCS_Node* Node)
		{
			if (!Node || Seen.Contains(Node))
			{
				return;
			}
			Seen.Add(Node);
			Rebuilt.Add(Node);
			for (USCS_Node* Child : Node->GetChildNodes())
			{
				Gather(Child);
			}
		};

		for (USCS_Node* Root : SCS->GetRootNodes())
		{
			Gather(Root);
		}

		if (FArrayProperty* AllNodesProp = FindFProperty<FArrayProperty>(USimpleConstructionScript::StaticClass(), TEXT("AllNodes")))
		{
			FScriptArrayHelper Helper(AllNodesProp, AllNodesProp->ContainerPtrToValuePtr<void>(SCS));
			Helper.EmptyValues();
			for (USCS_Node* Node : Rebuilt)
			{
				const int32 Index = Helper.AddValue();
				*reinterpret_cast<TObjectPtr<USCS_Node>*>(Helper.GetRawPtr(Index)) = Node;
			}
			Actions.Add(FString::Printf(TEXT("Rebuilt AllNodes (%d unique)"), Rebuilt.Num()));
		}
	}

	if (!bRebuildAllNodesOnly)
	{
		for (USCS_Node* Child : RootNode->GetChildNodes())
		{
			if (!Child || Child == BodyNode)
			{
				continue;
			}
			USceneComponent* ChildScene = Cast<USceneComponent>(Child->ComponentTemplate);
			if (!ChildScene)
			{
				continue;
			}
			ChildScene->Modify();
			const FTransform Corrected = InvBodyRel * ChildScene->GetRelativeTransform();
			ChildScene->SetRelativeLocation(Corrected.GetLocation());
			ChildScene->SetRelativeRotation(Corrected.Rotator());
			ChildScene->SetRelativeScale3D(Corrected.GetScale3D());
			Actions.Add(FString::Printf(TEXT("Un-baked extra %s scale from %s"), *ScaledComponentName, *Child->GetVariableName().ToString()));
		}
	}

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	FKismetEditorUtilities::CompileBlueprint(Blueprint);
	UEditorAssetLibrary::SaveLoadedAsset(Blueprint);
	Actions.Add(TEXT("Compiled and saved"));

	TSharedPtr<FJsonObject> ResultObj = BuildComponentHierarchyJson(Blueprint);
	ResultObj->SetStringField(TEXT("blueprint_path"), BlueprintPath);
	ResultObj->SetBoolField(TEXT("success"), true);
	TArray<TSharedPtr<FJsonValue>> ActionArr;
	for (const FString& A : Actions)
	{
		ActionArr.Add(MakeShared<FJsonValueString>(A));
	}
	ResultObj->SetArrayField(TEXT("actions"), ActionArr);
	return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleFixBlueprintPhysicsRoot(const TSharedPtr<FJsonObject>& Params)
{
	// HoverMovement / HoverThruster apply forces to Owner->GetRootComponent() as UPrimitiveComponent.
	// After the scaled-root fix, Root is a SceneComponent so forces no-op and child primitives
	// with SimulatePhysics fall independently. Replace Scene Root with an unscaled simulating
	// BoxComponent physics root; keep Body as a visual child (no simulate).
	FString BlueprintPath;
	if (!Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_path' parameter"));
	}

	FString PhysicsRootName = TEXT("PhysicsRoot");
	Params->TryGetStringField(TEXT("physics_root_name"), PhysicsRootName);

	FString BodyName = TEXT("Body");
	Params->TryGetStringField(TEXT("body_name"), BodyName);

	UBlueprint* Blueprint = Cast<UBlueprint>(UEditorAssetLibrary::LoadAsset(BlueprintPath));
	if (!Blueprint || !Blueprint->SimpleConstructionScript)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to load blueprint SCS"));
	}

	USimpleConstructionScript* SCS = Blueprint->SimpleConstructionScript;
	SCS->Modify();
	Blueprint->Modify();

	TArray<FString> Actions;

	USCS_Node* BodyNode = FindSCSNodeByName(SCS, FName(*BodyName));

	USCS_Node* SceneRootNode = FindSCSNodeByName(SCS, TEXT("Root"));
	if (!SceneRootNode)
	{
		for (USCS_Node* Node : SCS->GetRootNodes())
		{
			if (Node && Cast<USceneComponent>(Node->ComponentTemplate))
			{
				SceneRootNode = Node;
				break;
			}
		}
	}

	if (!SceneRootNode)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Could not find current scene root"));
	}

	USCS_Node* PhysicsRootNode = nullptr;

	const bool bSceneRootIsNonPrimitive =
		SceneRootNode->ComponentTemplate
		&& SceneRootNode->ComponentTemplate->IsA(USceneComponent::StaticClass())
		&& !SceneRootNode->ComponentTemplate->IsA(UPrimitiveComponent::StaticClass());

	if (!bSceneRootIsNonPrimitive && Cast<UPrimitiveComponent>(SceneRootNode->ComponentTemplate))
	{
		PhysicsRootNode = SceneRootNode;
		PhysicsRootName = SceneRootNode->GetVariableName().ToString();
		Actions.Add(FString::Printf(TEXT("Using existing primitive root %s"), *PhysicsRootName));
	}
	else
	{
		PhysicsRootNode = FindSCSNodeByName(SCS, FName(*PhysicsRootName));
		if (!PhysicsRootNode)
		{
			PhysicsRootNode = SCS->CreateNode(UBoxComponent::StaticClass(), FName(*PhysicsRootName));
			if (!PhysicsRootNode)
			{
				return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create PhysicsRoot BoxComponent"));
			}
			Actions.Add(TEXT("Created PhysicsRoot BoxComponent"));
		}

		UBoxComponent* Box = Cast<UBoxComponent>(PhysicsRootNode->ComponentTemplate);
		if (!Box)
		{
			return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("PhysicsRoot template is not a BoxComponent"));
		}

		Box->Modify();
		FVector BodyScale(2.75f, 1.0f, 1.0f);
		if (USceneComponent* BodyScene = BodyNode ? Cast<USceneComponent>(BodyNode->ComponentTemplate) : nullptr)
		{
			BodyScale = BodyScene->GetRelativeScale3D();
		}
		const FVector BoxExtent(50.0f * BodyScale.X, 50.0f * BodyScale.Y, 50.0f * BodyScale.Z);
		Box->SetBoxExtent(BoxExtent);
		Box->SetRelativeScale3D(FVector::OneVector);
		Box->SetRelativeLocation(FVector::ZeroVector);
		Box->SetRelativeRotation(FRotator::ZeroRotator);
		Box->SetCollisionProfileName(TEXT("PhysicsActor"));
		Box->SetSimulatePhysics(true);
		Box->SetEnableGravity(true);
		Box->BodyInstance.bSimulatePhysics = true;
		Box->SetHiddenInGame(true);
		Box->SetVisibility(false);
		Actions.Add(FString::Printf(TEXT("Configured PhysicsRoot BoxExtent=(%.1f,%.1f,%.1f)"), BoxExtent.X, BoxExtent.Y, BoxExtent.Z));

		if (UPrimitiveComponent* BodyPrim = BodyNode ? Cast<UPrimitiveComponent>(BodyNode->ComponentTemplate) : nullptr)
		{
			Box->SetLinearDamping(BodyPrim->GetLinearDamping());
			Box->SetAngularDamping(BodyPrim->GetAngularDamping());
			Actions.Add(TEXT("Copied damping from Body"));
		}

		if (USCS_Node* PhysParent = FindParentSCSNode(SCS, PhysicsRootNode))
		{
			PhysParent->RemoveChildNode(PhysicsRootNode, /*bRemoveFromAllNodes=*/true);
		}

		USCS_Node* OldRoot = SceneRootNode;
		if (OldRoot->IsRootNode())
		{
			SCS->RemoveNode(OldRoot, /*bValidateSceneRootNodes=*/false);
			Actions.Add(FString::Printf(TEXT("Removed %s from root list"), *OldRoot->GetVariableName().ToString()));
		}

		if (!PhysicsRootNode->IsRootNode())
		{
			SCS->AddNode(PhysicsRootNode);
			Actions.Add(TEXT("Promoted PhysicsRoot to scene root"));
		}

		PhysicsRootNode->MoveChildNodes(OldRoot);
		Actions.Add(FString::Printf(TEXT("Moved children from %s -> PhysicsRoot"), *OldRoot->GetVariableName().ToString()));

		if (bSceneRootIsNonPrimitive)
		{
			if (USCS_Node* P = FindParentSCSNode(SCS, OldRoot))
			{
				P->RemoveChildNode(OldRoot, true);
			}
			if (FArrayProperty* AllNodesProp = FindFProperty<FArrayProperty>(USimpleConstructionScript::StaticClass(), TEXT("AllNodes")))
			{
				FScriptArrayHelper Helper(AllNodesProp, AllNodesProp->ContainerPtrToValuePtr<void>(SCS));
				for (int32 i = Helper.Num() - 1; i >= 0; --i)
				{
					TObjectPtr<USCS_Node>* Elem = reinterpret_cast<TObjectPtr<USCS_Node>*>(Helper.GetRawPtr(i));
					if (Elem && *Elem == OldRoot)
					{
						Helper.RemoveValues(i);
					}
				}
			}
			Actions.Add(FString::Printf(TEXT("Removed old SceneComponent root '%s'"), *OldRoot->GetVariableName().ToString()));
		}
		else if (FindParentSCSNode(SCS, OldRoot) != PhysicsRootNode)
		{
			PhysicsRootNode->AddChildNode(OldRoot, true);
			OldRoot->SetParent(PhysicsRootNode);
			Actions.Add(FString::Printf(TEXT("Attached former root %s under PhysicsRoot"), *OldRoot->GetVariableName().ToString()));
		}
	}

	if (!PhysicsRootNode)
	{
		PhysicsRootNode = FindSCSNodeByName(SCS, FName(*PhysicsRootName));
	}
	if (!PhysicsRootNode)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Physics root node missing after promotion"));
	}

	for (USCS_Node* Node : SCS->GetAllNodes())
	{
		if (!Node || Node == PhysicsRootNode)
		{
			continue;
		}
		if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(Node->ComponentTemplate))
		{
			if (Prim->IsSimulatingPhysics() || Prim->BodyInstance.bSimulatePhysics)
			{
				Prim->Modify();
				Prim->SetSimulatePhysics(false);
				Prim->BodyInstance.bSimulatePhysics = false;
				Actions.Add(FString::Printf(TEXT("Disabled SimulatePhysics on %s"), *Node->GetVariableName().ToString()));
			}
		}
	}

	if (UPrimitiveComponent* PhysPrim = Cast<UPrimitiveComponent>(PhysicsRootNode->ComponentTemplate))
	{
		PhysPrim->Modify();
		PhysPrim->SetSimulatePhysics(true);
		PhysPrim->BodyInstance.bSimulatePhysics = true;
		PhysPrim->SetEnableGravity(true);
		Actions.Add(FString::Printf(TEXT("Enabled SimulatePhysics on %s"), *PhysicsRootNode->GetVariableName().ToString()));
	}
	else
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Physics root is not a PrimitiveComponent"));
	}

	{
		TArray<USCS_Node*> Rebuilt;
		TSet<USCS_Node*> Seen;
		TFunction<void(USCS_Node*)> Gather = [&](USCS_Node* Node)
		{
			if (!Node || Seen.Contains(Node))
			{
				return;
			}
			Seen.Add(Node);
			Rebuilt.Add(Node);
			for (USCS_Node* Child : Node->GetChildNodes())
			{
				Gather(Child);
			}
		};
		for (USCS_Node* Root : SCS->GetRootNodes())
		{
			Gather(Root);
		}
		if (FArrayProperty* AllNodesProp = FindFProperty<FArrayProperty>(USimpleConstructionScript::StaticClass(), TEXT("AllNodes")))
		{
			FScriptArrayHelper Helper(AllNodesProp, AllNodesProp->ContainerPtrToValuePtr<void>(SCS));
			Helper.EmptyValues();
			for (USCS_Node* Node : Rebuilt)
			{
				const int32 Index = Helper.AddValue();
				*reinterpret_cast<TObjectPtr<USCS_Node>*>(Helper.GetRawPtr(Index)) = Node;
			}
			Actions.Add(FString::Printf(TEXT("Rebuilt AllNodes (%d)"), Rebuilt.Num()));
		}
	}

	SCS->ValidateSceneRootNodes();
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	FKismetEditorUtilities::CompileBlueprint(Blueprint);
	UEditorAssetLibrary::SaveLoadedAsset(Blueprint);
	Actions.Add(TEXT("Compiled and saved"));

	TSharedPtr<FJsonObject> ResultObj = BuildComponentHierarchyJson(Blueprint);
	ResultObj->SetStringField(TEXT("blueprint_path"), BlueprintPath);
	ResultObj->SetBoolField(TEXT("success"), true);
	TArray<TSharedPtr<FJsonValue>> ActionArr;
	for (const FString& A : Actions)
	{
		ActionArr.Add(MakeShared<FJsonValueString>(A));
	}
	ResultObj->SetArrayField(TEXT("actions"), ActionArr);
	return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleSetBlueprintComponentTransform(const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintPath;
	if (!Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath))
	{
		FString BlueprintName;
		if (Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
		{
			BlueprintPath = FString::Printf(TEXT("/Game/Blueprints/%s"), *BlueprintName);
		}
		else
		{
			return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_path' or 'blueprint_name'"));
		}
	}

	FString ComponentName;
	if (!Params->TryGetStringField(TEXT("component_name"), ComponentName))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'component_name' parameter"));
	}

	UBlueprint* Blueprint = Cast<UBlueprint>(UEditorAssetLibrary::LoadAsset(BlueprintPath));
	if (!Blueprint)
	{
		Blueprint = FEpicUnrealMCPCommonUtils::FindBlueprint(BlueprintPath);
	}
	if (!Blueprint || !Blueprint->SimpleConstructionScript)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load blueprint: %s"), *BlueprintPath));
	}

	USCS_Node* TargetNode = nullptr;
	for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
	{
		if (Node && Node->GetVariableName().ToString() == ComponentName)
		{
			TargetNode = Node;
			break;
		}
	}
	if (!TargetNode)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Component not found: %s"), *ComponentName));
	}

	USceneComponent* Scene = Cast<USceneComponent>(TargetNode->ComponentTemplate);
	if (!Scene)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Component is not a SceneComponent: %s"), *ComponentName));
	}

	const FVector OldLoc = Scene->GetRelativeLocation();
	const FRotator OldRot = Scene->GetRelativeRotation();
	const FVector OldScale = Scene->GetRelativeScale3D();

	if (Params->HasField(TEXT("location")))
	{
		Scene->SetRelativeLocation(FEpicUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("location")));
	}
	if (Params->HasField(TEXT("rotation")))
	{
		// rotation array is [pitch, yaw, roll] (same as other MCP set commands)
		Scene->SetRelativeRotation(FEpicUnrealMCPCommonUtils::GetRotatorFromJson(Params, TEXT("rotation")));
	}
	if (Params->HasField(TEXT("scale")))
	{
		Scene->SetRelativeScale3D(FEpicUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("scale")));
	}

	const bool bCompile = !Params->HasField(TEXT("compile")) || Params->GetBoolField(TEXT("compile"));
	const bool bSave = Params->HasField(TEXT("save")) && Params->GetBoolField(TEXT("save"));

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	if (bCompile)
	{
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
	}
	if (bSave)
	{
		UEditorAssetLibrary::SaveLoadedAsset(Blueprint);
	}

	const FVector NewLoc = Scene->GetRelativeLocation();
	const FRotator NewRot = Scene->GetRelativeRotation();
	const FVector NewScale = Scene->GetRelativeScale3D();

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("blueprint_path"), BlueprintPath);
	ResultObj->SetStringField(TEXT("component_name"), ComponentName);

	auto MakeVec = [](const FVector& V)
	{
		TArray<TSharedPtr<FJsonValue>> Arr{
			MakeShared<FJsonValueNumber>(V.X),
			MakeShared<FJsonValueNumber>(V.Y),
			MakeShared<FJsonValueNumber>(V.Z)
		};
		return Arr;
	};
	auto MakeRot = [](const FRotator& R)
	{
		// Dump as [roll, pitch, yaw] to match hierarchy dump convention
		TArray<TSharedPtr<FJsonValue>> Arr{
			MakeShared<FJsonValueNumber>(R.Roll),
			MakeShared<FJsonValueNumber>(R.Pitch),
			MakeShared<FJsonValueNumber>(R.Yaw)
		};
		return Arr;
	};

	TSharedPtr<FJsonObject> Before = MakeShared<FJsonObject>();
	Before->SetArrayField(TEXT("location"), MakeVec(OldLoc));
	Before->SetArrayField(TEXT("rotation"), MakeRot(OldRot));
	Before->SetArrayField(TEXT("scale"), MakeVec(OldScale));
	ResultObj->SetObjectField(TEXT("before"), Before);

	TSharedPtr<FJsonObject> After = MakeShared<FJsonObject>();
	After->SetArrayField(TEXT("location"), MakeVec(NewLoc));
	After->SetArrayField(TEXT("rotation"), MakeRot(NewRot));
	After->SetArrayField(TEXT("scale"), MakeVec(NewScale));
	ResultObj->SetObjectField(TEXT("after"), After);

	return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleSetBlueprintComponentAbsolute(const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintPath;
	if (!Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath))
	{
		FString BlueprintName;
		if (Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
		{
			BlueprintPath = FString::Printf(TEXT("/Game/Blueprints/%s"), *BlueprintName);
		}
		else
		{
			return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_path' or 'blueprint_name'"));
		}
	}

	FString ComponentName;
	if (!Params->TryGetStringField(TEXT("component_name"), ComponentName))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'component_name' parameter"));
	}

	UBlueprint* Blueprint = Cast<UBlueprint>(UEditorAssetLibrary::LoadAsset(BlueprintPath));
	if (!Blueprint)
	{
		Blueprint = FEpicUnrealMCPCommonUtils::FindBlueprint(BlueprintPath);
	}
	if (!Blueprint || !Blueprint->SimpleConstructionScript)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load blueprint: %s"), *BlueprintPath));
	}

	USCS_Node* TargetNode = FindSCSNodeByName(Blueprint->SimpleConstructionScript, FName(*ComponentName));
	if (!TargetNode)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Component not found: %s"), *ComponentName));
	}

	USceneComponent* Scene = Cast<USceneComponent>(TargetNode->ComponentTemplate);
	if (!Scene)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Component is not a SceneComponent: %s"), *ComponentName));
	}

	bool bAbsoluteLocation = Scene->IsUsingAbsoluteLocation();
	bool bAbsoluteRotation = Scene->IsUsingAbsoluteRotation();
	bool bAbsoluteScale = Scene->IsUsingAbsoluteScale();
	Params->TryGetBoolField(TEXT("absolute_location"), bAbsoluteLocation);
	Params->TryGetBoolField(TEXT("absolute_rotation"), bAbsoluteRotation);
	Params->TryGetBoolField(TEXT("absolute_scale"), bAbsoluteScale);

	const bool bOldAbsLocation = Scene->IsUsingAbsoluteLocation();
	const bool bOldAbsRotation = Scene->IsUsingAbsoluteRotation();
	const bool bOldAbsScale = Scene->IsUsingAbsoluteScale();

	Scene->Modify();
	Scene->SetAbsolute(bAbsoluteLocation, bAbsoluteRotation, bAbsoluteScale);

	const bool bCompile = !Params->HasField(TEXT("compile")) || Params->GetBoolField(TEXT("compile"));
	const bool bSave = Params->HasField(TEXT("save")) && Params->GetBoolField(TEXT("save"));

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	if (bCompile)
	{
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
	}
	if (bSave)
	{
		UEditorAssetLibrary::SaveLoadedAsset(Blueprint);
	}

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("blueprint_path"), BlueprintPath);
	ResultObj->SetStringField(TEXT("component_name"), ComponentName);
	ResultObj->SetBoolField(TEXT("old_absolute_location"), bOldAbsLocation);
	ResultObj->SetBoolField(TEXT("old_absolute_rotation"), bOldAbsRotation);
	ResultObj->SetBoolField(TEXT("old_absolute_scale"), bOldAbsScale);
	ResultObj->SetBoolField(TEXT("new_absolute_location"), Scene->IsUsingAbsoluteLocation());
	ResultObj->SetBoolField(TEXT("new_absolute_rotation"), Scene->IsUsingAbsoluteRotation());
	ResultObj->SetBoolField(TEXT("new_absolute_scale"), Scene->IsUsingAbsoluteScale());
	return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleReparentBlueprintComponent(const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintPath;
	if (!Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath))
	{
		FString BlueprintName;
		if (Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
		{
			BlueprintPath = FString::Printf(TEXT("/Game/Blueprints/%s"), *BlueprintName);
		}
		else
		{
			return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_path' or 'blueprint_name'"));
		}
	}

	FString ComponentName;
	FString NewParentName;
	if (!Params->TryGetStringField(TEXT("component_name"), ComponentName))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'component_name' parameter"));
	}
	if (!Params->TryGetStringField(TEXT("new_parent_name"), NewParentName))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'new_parent_name' parameter"));
	}

	UBlueprint* Blueprint = Cast<UBlueprint>(UEditorAssetLibrary::LoadAsset(BlueprintPath));
	if (!Blueprint)
	{
		Blueprint = FEpicUnrealMCPCommonUtils::FindBlueprint(BlueprintPath);
	}
	if (!Blueprint || !Blueprint->SimpleConstructionScript)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load blueprint: %s"), *BlueprintPath));
	}

	USimpleConstructionScript* SCS = Blueprint->SimpleConstructionScript;
	USCS_Node* ChildNode = FindSCSNodeByName(SCS, FName(*ComponentName));
	USCS_Node* NewParentNode = FindSCSNodeByName(SCS, FName(*NewParentName));
	if (!ChildNode)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Component not found: %s"), *ComponentName));
	}
	if (!NewParentNode)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("New parent not found: %s"), *NewParentName));
	}
	if (ChildNode == NewParentNode)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Component cannot be parented to itself"));
	}

	FString OldParentName = TEXT("");
	if (USCS_Node* OldParent = FindParentSCSNode(SCS, ChildNode))
	{
		OldParentName = OldParent->GetVariableName().ToString();
		if (OldParent != NewParentNode)
		{
			OldParent->RemoveChildNode(ChildNode, /*bRemoveFromAllNodes=*/false);
		}
	}
	else if (ChildNode->IsRootNode())
	{
		SCS->RemoveNode(ChildNode, /*bValidateSceneRootNodes=*/false);
		OldParentName = TEXT("<root>");
	}

	if (FindParentSCSNode(SCS, ChildNode) != NewParentNode)
	{
		NewParentNode->AddChildNode(ChildNode, /*bAddToAllNodes=*/true);
		ChildNode->SetParent(NewParentNode);
	}

	SCS->ValidateSceneRootNodes();

	const bool bCompile = !Params->HasField(TEXT("compile")) || Params->GetBoolField(TEXT("compile"));
	const bool bSave = Params->HasField(TEXT("save")) && Params->GetBoolField(TEXT("save"));
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	if (bCompile)
	{
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
	}
	if (bSave)
	{
		UEditorAssetLibrary::SaveLoadedAsset(Blueprint);
	}

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("blueprint_path"), BlueprintPath);
	ResultObj->SetStringField(TEXT("component_name"), ComponentName);
	ResultObj->SetStringField(TEXT("old_parent_name"), OldParentName);
	ResultObj->SetStringField(TEXT("new_parent_name"), NewParentName);
	return ResultObj;
}
