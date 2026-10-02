// Andy ESP32 Diagnostics - Roll call node registry

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AndyDiagTypes.h"
#include "AndyDiagRegistry.generated.h"

/** Known ESP32 node in the ship roll call (stable ID + display label). */
USTRUCT(BlueprintType)
struct FAndyDiagNodeDefinition
{
	GENERATED_BODY()

	/** Serial/firmware node ID sent in DIAG_NODE lines (e.g. ANDY, PORT, ENG_FL). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Andy|Diagnostics")
	FName NodeId;

	/** Human-readable label for UI. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Andy|Diagnostics")
	FText DisplayLabel;

	/** Sort order in roll-call lists (lower = higher on screen). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Andy|Diagnostics")
	int32 SortOrder = 0;
};

/**
 * Default roll-call registry for Andy ship ESP32 nodes.
 * Firmware should use the NodeId strings in DIAG_NODE lines.
 */
UCLASS()
class ARDUINOCOMMUNICATION_API UAndyDiagRegistry : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Full default roll call (10 nodes). */
	UFUNCTION(BlueprintPure, Category = "Andy|Diagnostics|RollCall")
	static TArray<FAndyDiagNodeDefinition> GetDefaultRollCall();

	/** Default roll-call node IDs in display order. */
	UFUNCTION(BlueprintPure, Category = "Andy|Diagnostics|RollCall")
	static TArray<FName> GetDefaultRollCallNodeIds();

	/** Display label for a node ID, or the raw ID if unknown. */
	UFUNCTION(BlueprintPure, Category = "Andy|Diagnostics|RollCall")
	static FText GetDisplayLabel(FName NodeId);

	/** Map firmware alias (e.g. WEAPON_PORT) to canonical roll-call ID (e.g. PORT). */
	UFUNCTION(BlueprintPure, Category = "Andy|Diagnostics|RollCall")
	static FName CanonicalizeNodeId(FName RawNodeId);

	/** True if this ID is one of the known ship roll-call nodes (canonical or alias). */
	UFUNCTION(BlueprintPure, Category = "Andy|Diagnostics|RollCall")
	static bool IsKnownRollCallNode(FName NodeId);

	/** Normalize node info to canonical ID + display name for UI storage. */
	UFUNCTION(BlueprintPure, Category = "Andy|Diagnostics|RollCall")
	static FAndyDiagNodeInfo NormalizeNodeInfo(const FAndyDiagNodeInfo& Node);

	/** Placeholder entry when a node did not respond in the report. */
	UFUNCTION(BlueprintPure, Category = "Andy|Diagnostics|RollCall")
	static FAndyDiagNodeInfo MakeMissingNode(FName NodeId, EAndyDiagStatus Status = EAndyDiagStatus::Offline);

	/**
	 * Merge a diagnostic report with an expected roll call.
	 * Returns exactly one entry per expected node (aliases deduplicated).
	 * During in-progress runs, pass StickyReport (usually the last completed report)
	 * so rows don't flash "--" while waiting for the next DIAG_NODE line.
	 */
	UFUNCTION(BlueprintPure, Category = "Andy|Diagnostics|RollCall")
	static TArray<FAndyDiagNodeInfo> BuildOrderedRollCall(
		const FAndyDiagReport& Report,
		const TArray<FName>& ExpectedNodeIds,
		const FAndyDiagReport& StickyReport);

	/**
	 * Fill gaps in NewReport with nodes from PreviousReport (same canonical IDs).
	 * Used when a serial roll call loses DIAG_NODE lines but hardware is still fine.
	 */
	UFUNCTION(BlueprintPure, Category = "Andy|Diagnostics|RollCall")
	static FAndyDiagReport MergeWithPreviousReport(
		const FAndyDiagReport& NewReport,
		const FAndyDiagReport& PreviousReport,
		const TArray<FName>& ExpectedNodeIds);

	/** Format a single node line for UI: "Weapon Port: OK" */
	UFUNCTION(BlueprintPure, Category = "Andy|Diagnostics|RollCall")
	static FString FormatNodeStatusLine(const FAndyDiagNodeInfo& Node);

	/** Status color for UI widgets. */
	UFUNCTION(BlueprintPure, Category = "Andy|Diagnostics|RollCall")
	static FLinearColor GetStatusColor(EAndyDiagStatus Status);

private:
	static const TArray<FAndyDiagNodeDefinition>& GetDefaultDefinitions();
	static const TMap<FName, FName>& GetAliasToCanonicalMap();
	static TArray<FName> GetAliasesForCanonical(FName CanonicalId);
	static bool FindNodeForRollCall(const FAndyDiagReport& Report, FName CanonicalId, FAndyDiagNodeInfo& OutNode);
};
