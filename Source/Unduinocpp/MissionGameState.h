// Mission Game State - Replicated mission and threat state
// Server-authoritative; MissionManagerSubsystem (server) writes here; clients read.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "MissionTypes.h"
#include "MissionGameState.generated.h"

/** Broadcast on clients when MissionStates or ObjectiveProgress replicate. Bind UI refresh here. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnReplicatedMissionDataChanged);

/** Replicated mission state entry (parallel arrays on GameState) */
USTRUCT(BlueprintType)
struct FReplicatedMissionState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FName MissionID;

	UPROPERTY(BlueprintReadOnly)
	uint8 State = 0; // EMissionState
};

/** Replicated faction threat entry (parallel arrays) */
USTRUCT(BlueprintType)
struct FReplicatedFactionThreat
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FName FactionID;

	UPROPERTY(BlueprintReadOnly)
	int32 ThreatLevel = 0;
};

/**
 * Game State that holds replicated mission and threat data.
 * MissionManagerSubsystem (server only) updates this state; all clients receive it.
 * No mission logic here—only storage and replication.
 */
UCLASS()
class UNDUINOCPP_API AMissionGameState : public AGameState
{
	GENERATED_BODY()

public:
	AMissionGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// --- Threat (replicated) ---

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Mission|Threat")
	int32 GlobalThreatLevel = 0;

	/** Per-faction threat (replicated as array) */
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Mission|Threat")
	TArray<FReplicatedFactionThreat> FactionThreatLevels;

	// --- Mission state (replicated) ---

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MissionStates, Replicated, Category = "Mission")
	TArray<FReplicatedMissionState> MissionStates;

	/** Objective progress (replicated). OnRep fires on clients when server updates replicate. */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_ObjectiveProgress, Replicated, Category = "Mission")
	TArray<FMissionObjectiveProgress> ObjectiveProgress;

	/** Fires on clients when MissionStates or ObjectiveProgress replicate. Bind your mission UI refresh to this in multiplayer. */
	UPROPERTY(BlueprintAssignable, Category = "Mission")
	FOnReplicatedMissionDataChanged OnReplicatedMissionDataChanged;

	// --- Helpers (no replication) ---

	EMissionState GetMissionState(FName MissionID) const;
	int32 GetFactionThreatLevel(FName FactionID) const;
	void GetObjectiveProgress(FName MissionID, int32 ObjectiveIndex, bool& bFound, FMissionObjectiveProgress& OutProgress) const;

private:
	UFUNCTION()
	void OnRep_MissionStates();
	UFUNCTION()
	void OnRep_ObjectiveProgress();
};
