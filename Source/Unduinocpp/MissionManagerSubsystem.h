// Mission Manager Subsystem - Central mission and threat controller
// GameInstanceSubsystem: persists across level loads. Server-authoritative for state changes.

#pragma once

#include "CoreMinimal.h"
#include "Tickable.h"
#include "Engine/DataAsset.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MissionTypes.h"
#include "MissionDataAsset.h"
#include "MissionGameState.h"
#include "MissionManagerSubsystem.generated.h"

class APlayerState;
class APawn;
class AMissionPlayerState;

/** Generic world event by name (e.g. AudioEvent_ThreatIncreased). Name-only; no middleware. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWorldEvent, FName, EventName);

class UMissionRegistryAsset;
class UWorld;

// --- Delegates for world systems (spawn, AI, audio hooks) ---

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnThreatChanged, int32, NewThreatLevel, int32, PreviousThreatLevel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnThreatThresholdCrossed, int32, NewThreatLevel, int32, Threshold, bool, bAboveThreshold);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMissionActivated, FName, MissionID, UMissionDataAsset*, MissionData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnMissionCompleted, FName, MissionID, bool, bSuccess, EMissionState, FinalState, UMissionDataAsset*, MissionData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnObjectiveProgress, FName, MissionID, int32, ObjectiveIndex, int32, CurrentCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnPerPlayerObjectiveProgress, APlayerState*, PlayerState, FName, MissionID, int32, ObjectiveIndex, int32, CurrentCount);

// --- UI: designer-friendly mission/objective summaries ---

/** Single objective summary for UI text. Use FormattedDisplayText for the line to show (e.g. "Visit Waypoint" or "Kill 5 monsters (0/5)"). */
USTRUCT(BlueprintType)
struct FMissionObjectiveSummaryForUI
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Mission|UI")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "Mission|UI")
	int32 CurrentCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Mission|UI")
	int32 TargetCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Mission|UI")
	bool bCompleted = false;

	UPROPERTY(BlueprintReadOnly, Category = "Mission|UI")
	bool bIsPerPlayer = false;

	/** Ready-to-display line: "Visit Waypoint" when TargetCount <= 1, or "Kill 5 monsters (0/5)" when TargetCount > 1. Use bCompleted for a checkmark/icon. */
	UPROPERTY(BlueprintReadOnly, Category = "Mission|UI")
	FText FormattedDisplayText;
};

/** One active mission with all its objectives for UI. */
USTRUCT(BlueprintType)
struct FMissionSummaryForUI
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Mission|UI")
	FName MissionID;

	UPROPERTY(BlueprintReadOnly, Category = "Mission|UI")
	FText MissionDisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "Mission|UI")
	TArray<FMissionObjectiveSummaryForUI> Objectives;
};

/**
 * Mission registry: list of mission data assets and which one is the main mission.
 * Set this on your GameMode; MissionManager reads it at session start (when RegisterMissionsFromRegistry is called).
 */
UCLASS(BlueprintType)
class UNDUINOCPP_API UMissionRegistryAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** All missions in this session (Main + Sub + Side). Loaded and registered when RegisterMissionsFromRegistry is called. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission Registry")
	TArray<TObjectPtr<UMissionDataAsset>> Missions;

	/** MissionID of the main mission (must be in Missions). This one starts Active at session start. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission Registry")
	FName MainMissionID;

	/** Optional threat thresholds to fire OnThreatThresholdCrossed (e.g. 25, 50, 75). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission Registry|Threat")
	TArray<int32> ThreatThresholds;
};

/**
 * Game Instance Subsystem: Mission and threat management.
 * - Registers missions from a MissionRegistryAsset (call from GameMode BeginPlay).
 * - Tracks state in MissionGameState (replicated); server only writes.
 * - Evaluates visibility, time limits, and fail-forward actions.
 * - Broadcasts events for spawn/AI/audio systems.
 */
UCLASS(BlueprintType)
class UNDUINOCPP_API UMissionManagerSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	UMissionManagerSubsystem();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;
	virtual UWorld* GetTickableGameObjectWorld() const override;

	// --- Registration (call from GameMode BeginPlay; server and clients can call, but only server applies) ---

	/**
	 * Register all missions from the registry and start the main mission.
	 * Call once per session (e.g. from GameMode::BeginPlay).
	 */
	UFUNCTION(BlueprintCallable, Category = "Mission|Registration")
	void RegisterMissionsFromRegistry(UMissionRegistryAsset* Registry);

	/** Optional: add a single mission asset (e.g. for dynamically loaded side missions). */
	UFUNCTION(BlueprintCallable, Category = "Mission|Registration")
	void RegisterMission(UMissionDataAsset* MissionData);

	// --- Queries (read-only; work on client from replicated GameState) ---

	UFUNCTION(BlueprintPure, Category = "Mission")
	EMissionState GetMissionState(FName MissionID) const;

	UFUNCTION(BlueprintPure, Category = "Mission")
	int32 GetGlobalThreatLevel() const;

	UFUNCTION(BlueprintPure, Category = "Mission")
	int32 GetFactionThreatLevel(FName FactionID) const;

	UFUNCTION(BlueprintPure, Category = "Mission")
	AMissionGameState* GetMissionGameState() const;

	/** Get mission definition by ID (from registered assets). */
	UFUNCTION(BlueprintPure, Category = "Mission")
	UMissionDataAsset* GetMissionData(FName MissionID) const;

	// --- Server-only: state changes (no-op on client) ---

	/** Report objective progress. RequiredRole is validated if objective has one. Session-scoped objectives only; PerPlayer objectives use ReportObjectiveProgressForPlayer. */
	UFUNCTION(BlueprintCallable, Category = "Mission|Server")
	void ReportObjectiveProgress(FName MissionID, int32 ObjectiveIndex, int32 DeltaCount, EMissionRole ReporterRole);

	/**
	 * Report progress for a PerPlayer-scoped objective. Call from server (or RPC to server) when e.g. this player's pawn hits a waypoint.
	 * Requires PlayerState to be (or be cast to) MissionPlayerState — set DefaultPlayerStateClass on GameMode to MissionPlayerState.
	 */
	UFUNCTION(BlueprintCallable, Category = "Mission|Server")
	void ReportObjectiveProgressForPlayer(APlayerState* PlayerState, FName MissionID, int32 ObjectiveIndex, int32 DeltaCount, EMissionRole ReporterRole);

	/** Convenience: get PlayerState from Pawn and call ReportObjectiveProgressForPlayer. */
	UFUNCTION(BlueprintCallable, Category = "Mission|Server")
	void ReportObjectiveProgressForPawn(APawn* Pawn, FName MissionID, int32 ObjectiveIndex, int32 DeltaCount, EMissionRole ReporterRole);

	/** Read per-player progress for UI (works on owning client after replication). */
	UFUNCTION(BlueprintPure, Category = "Mission")
	void GetPerPlayerObjectiveProgress(APlayerState* PlayerState, FName MissionID, int32 ObjectiveIndex, bool& bFound, FMissionObjectiveProgress& OutProgress) const;

	/**
	 * Get all currently active missions with objective progress for UI text.
	 * Pass the local player's Player State (e.g. from widget: Get Owning Player -> Get Player State) so per-player objectives show this player's progress.
	 * Call this when building your mission readout; bind to OnObjectiveProgress and OnPerPlayerObjectiveProgress to refresh when progress changes.
	 */
	UFUNCTION(BlueprintPure, Category = "Mission|UI")
	TArray<FMissionSummaryForUI> GetActiveMissionsForUI(APlayerState* ForPlayer) const;

	/** Set mission state (e.g. activate, complete, expire). Server only. */
	UFUNCTION(BlueprintCallable, Category = "Mission|Server")
	void SetMissionState(FName MissionID, EMissionState NewState);

	/** Add threat (global and/or faction). Server only. */
	UFUNCTION(BlueprintCallable, Category = "Mission|Server")
	void AddThreat(int32 GlobalDelta, const TMap<FName, int32>& FactionDelta);

	/** Evaluate visibility for all missions and update state. Call periodically or on relevant triggers. */
	UFUNCTION(BlueprintCallable, Category = "Mission|Server")
	void EvaluateVisibilityConditions();

	// --- Events (subscribe from spawn system, AI director, audio hooks) ---

	UPROPERTY(BlueprintAssignable, Category = "Mission|Events")
	FOnThreatChanged OnThreatChanged;

	UPROPERTY(BlueprintAssignable, Category = "Mission|Events")
	FOnThreatThresholdCrossed OnThreatThresholdCrossed;

	UPROPERTY(BlueprintAssignable, Category = "Mission|Events")
	FOnMissionActivated OnMissionActivated;

	UPROPERTY(BlueprintAssignable, Category = "Mission|Events")
	FOnMissionCompleted OnMissionCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Mission|Events")
	FOnObjectiveProgress OnObjectiveProgress;

	UPROPERTY(BlueprintAssignable, Category = "Mission|Events")
	FOnPerPlayerObjectiveProgress OnPerPlayerObjectiveProgress;

	/** Generic world event (e.g. AudioEvent_ThreatIncreased). Name-only; no middleware. */
	UPROPERTY(BlueprintAssignable, Category = "Mission|Events")
	FOnWorldEvent OnWorldEvent;

protected:
	/** Cached mission game state (from current world). */
	UPROPERTY()
	TWeakObjectPtr<AMissionGameState> CachedMissionGameState;

	/** Registered mission definitions by MissionID */
	UPROPERTY()
	TMap<FName, TObjectPtr<UMissionDataAsset>> MissionDataMap;

	/** Last known threat for threshold detection */
	int32 LastBroadcastThreatLevel = -1;

	/** Thresholds to broadcast (from registry) */
	UPROPERTY()
	TArray<int32> ThreatThresholds;

	/** Session time elapsed (for visibility TimeElapsedSeconds) */
	float SessionTimeSeconds = 0.0f;

	/** When each mission became Active (server only; for mission-level time limit) */
	TMap<FName, double> MissionActiveStartTime;

	AMissionGameState* GetOrFindMissionGameState() const;
	void EnsureMissionStateInReplicatedArrays(FName MissionID, EMissionState State);
	void EnsureFactionThreatInReplicatedArrays(FName FactionID, int32 Level);
	void ApplyMissionActions(const TArray<FMissionAction>& Actions, bool bSuccess);
	void CompleteMission(FName MissionID, bool bSuccess);
	bool EvaluateVisibilityCondition(const FMissionVisibilityCondition& Cond) const;
	bool EvaluateAllVisibilityConditions(const UMissionDataAsset* Mission) const;
	void CheckThreatThresholds(int32 PreviousThreat, int32 NewThreat);
	void BroadcastWorldEvent(FName EventName);
	bool HasAuthority() const;

	/** Session objectives only: all rows for this mission in GameState are complete. */
	bool AreAllSessionObjectivesComplete(FName MissionID, const UMissionDataAsset* Data, AMissionGameState* GS) const;
	/** If mission has bCompleteWhenAllPlayersFinishPerPlayerObjectives, true when every player has finished all PerPlayer objectives. */
	bool AreAllPlayersPerPlayerObjectivesComplete(FName MissionID, const UMissionDataAsset* Data) const;
};
