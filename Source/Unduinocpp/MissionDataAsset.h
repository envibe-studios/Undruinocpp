// Mission Data Asset - Data-driven mission definition
// Designers add missions entirely through Data Assets; no code changes required.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MissionTypes.h"
#include "MissionDataAsset.generated.h"

/**
 * Primary Data Asset defining a single mission.
 * Supports Main, Sub, and Side missions with objectives, visibility conditions,
 * and fail-forward OnSuccessActions / OnFailureActions.
 */
UCLASS(BlueprintType)
class UNDUINOCPP_API UMissionDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Unique mission identifier (used for ParentMissionID, VisibilityConditions, and actions) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Identity")
	FName MissionID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Identity")
	EMissionType MissionType = EMissionType::Main;

	/** Parent mission ID for Submissions. Empty for Main and optional for Side. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Identity")
	FName ParentMissionID;

	/** Optional faction for faction-based threat and variation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Identity")
	FName FactionID;

	/** Objectives to complete (in order unless design specifies otherwise) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Objectives")
	TArray<FMissionObjectiveDef> Objectives;

	/** Conditions that must be met for this mission to become visible (Available). Evaluated by MissionManager. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Visibility")
	TArray<FMissionVisibilityCondition> VisibilityConditions;

	/** Time limit for the whole mission in seconds. 0 = no limit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Time", meta = (ClampMin = "0.0"))
	float TimeLimitSeconds = 0.0f;

	/** Actions executed when mission completes successfully (unlock missions, threat delta, events) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Fail-Forward")
	TArray<FMissionAction> OnSuccessActions;

	/** Actions executed when mission fails or expires (fail-forward: escalate threat, unlock side missions) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Fail-Forward")
	TArray<FMissionAction> OnFailureActions;

	/** Threat delta applied on success (convenience; can also use OnSuccessActions) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Fail-Forward")
	int32 ThreatDeltaOnSuccess = 0;

	/** Threat delta applied on failure (convenience; can also use OnFailureActions) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Fail-Forward")
	int32 ThreatDeltaOnFailure = 0;

	// --- Replayability ---

	/** If true, TargetCount for objectives can be randomized between MinTargetCount and MaxTargetCount */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Replayability")
	bool bRandomizeTargetCount = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Replayability", meta = (EditCondition = "bRandomizeTargetCount", ClampMin = "0"))
	int32 MinTargetCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Replayability", meta = (EditCondition = "bRandomizeTargetCount", ClampMin = "0"))
	int32 MaxTargetCount = 1;

	/** Optional display name for UI or logs */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Display")
	FText DisplayName;

	// --- Per-player completion ---

	/**
	 * If true, mission completes successfully only when every connected player has completed
	 * all PerPlayer-scoped objectives for this mission (and all Session objectives are already done).
	 * Use for "everyone must finish the lap" without a single shared counter.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|PerPlayer")
	bool bCompleteWhenAllPlayersFinishPerPlayerObjectives = false;

	// --- PrimaryDataAsset ---

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(FName("MissionDataAsset"), MissionID.IsNone() ? FName("Unknown") : MissionID);
	}
};
