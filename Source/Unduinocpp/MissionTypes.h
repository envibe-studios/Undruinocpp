// Mission System - Core types, enums, and role definitions
// Used by MissionDataAsset and MissionManagerSubsystem

#pragma once

#include "CoreMinimal.h"
#include "MissionTypes.generated.h"

/** Faction identifier for per-faction threat and mission variation. Optional. */
using FFactionID = FName;

/** Mission state lifecycle */
UENUM(BlueprintType)
enum class EMissionState : uint8
{
	Hidden           UMETA(DisplayName = "Hidden"),
	Available        UMETA(DisplayName = "Available"),
	Active           UMETA(DisplayName = "Active"),
	Completed_Success UMETA(DisplayName = "Completed (Success)"),
	Completed_Failure UMETA(DisplayName = "Completed (Failure)"),
	Expired          UMETA(DisplayName = "Expired")
};

/** Mission type in the session hierarchy */
UENUM(BlueprintType)
enum class EMissionType : uint8
{
	Main  UMETA(DisplayName = "Main"),
	Sub   UMETA(DisplayName = "Sub"),
	Side  UMETA(DisplayName = "Side")
};

/**
 * Who owns objective progress for this objective.
 * Session = one shared progress bar (co-op / first-to-trigger).
 * PerPlayer = each player has their own progress (races, individual waypoints).
 */
UENUM(BlueprintType)
enum class EObjectiveScope : uint8
{
	Session   UMETA(DisplayName = "Session (shared)"),
	PerPlayer UMETA(DisplayName = "Per player")
};

/** Role required for objective progress (role-specific objectives) */
UENUM(BlueprintType)
enum class EMissionRole : uint8
{
	Any     UMETA(DisplayName = "Any"),
	Pilot   UMETA(DisplayName = "Pilot"),
	Gunner  UMETA(DisplayName = "Gunner"),
	Engineer UMETA(DisplayName = "Engineer")
};

/** Objective types supported by the mission system. Designers drive behavior via data. */
UENUM(BlueprintType)
enum class EObjectiveType : uint8
{
	// Exploration
	Exploration              UMETA(DisplayName = "Exploration"),
	EnterCollider            UMETA(DisplayName = "Enter Collider"),
	RemainInZoneForDuration  UMETA(DisplayName = "Remain In Zone For Duration"),
	// Combat
	DestroyTarget            UMETA(DisplayName = "Destroy Target"),
	DestroyClassCount        UMETA(DisplayName = "Destroy Class Count"),
	TimedKill                UMETA(DisplayName = "Timed Kill (X in Y seconds)"),
	// Resource
	CollectResourceCount     UMETA(DisplayName = "Collect Resource Count"),
	DeliverResourceToZone    UMETA(DisplayName = "Deliver Resource To Zone"),
	// Ship interaction
	RepairSystem             UMETA(DisplayName = "Repair System"),
	ReloadAmmo               UMETA(DisplayName = "Reload Ammo"),
	RestoreOxygenAboveThreshold UMETA(DisplayName = "Restore Oxygen Above Threshold"),
	MaintainSystemAboveThresholdForDuration UMETA(DisplayName = "Maintain System Above Threshold For Duration"),
	// Survival
	SurviveDuration          UMETA(DisplayName = "Survive Duration"),
	DefendTarget             UMETA(DisplayName = "Defend Target"),
	// State-based
	MaintainOxygenAboveThreshold UMETA(DisplayName = "Maintain Oxygen Above Threshold"),
	MaintainHullIntegrity    UMETA(DisplayName = "Maintain Hull Integrity"),
	MaintainThrusterHealth   UMETA(DisplayName = "Maintain Thruster Health")
};

/** Single objective definition within a mission (data-driven) */
USTRUCT(BlueprintType)
struct FMissionObjectiveDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	EObjectiveType ObjectiveType = EObjectiveType::Exploration;

	/** Optional: actor class to count or target (e.g. for DestroyClassCount) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	TSoftClassPtr<AActor> TargetActorClass;

	/** Target count to complete (e.g. destroy 3, collect 5). Can be randomized at runtime if mission uses Min/Max. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective", meta = (ClampMin = "0"))
	int32 TargetCount = 1;

	/**
	 * Session = progress is shared on GameState (current default behavior).
	 * PerPlayer = progress is stored per PlayerState; use ReportObjectiveProgressForPlayer from waypoints/overlaps.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	EObjectiveScope ObjectiveScope = EObjectiveScope::Session;

	/** Role that must perform this objective for progress to count. Any = no restriction. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	EMissionRole RequiredRole = EMissionRole::Any;

	/** Optional time limit in seconds for this objective. 0 = no limit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective", meta = (ClampMin = "0.0"))
	float TimeLimitSeconds = 0.0f;

	/** Optional zone/actor name or tag for EnterCollider, RemainInZone, DeliverResourceToZone, DefendTarget */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	FName ZoneReference;

	/** Optional: required state threshold (e.g. oxygen above 50). Interpretation depends on ObjectiveType. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float StateThresholdPercent = 0.0f;

	/** Optional display name for UI or logs */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	FText DisplayName;
};

/** Runtime progress for one objective (replicated) */
USTRUCT(BlueprintType)
struct FMissionObjectiveProgress
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission")
	FName MissionID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission")
	int32 ObjectiveIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission")
	int32 CurrentCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission")
	bool bCompleted = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission")
	float TimeRemaining = 0.0f;
};

/** Condition for mission visibility (when to unhide or offer a mission) */
USTRUCT(BlueprintType)
struct FMissionVisibilityCondition
{
	GENERATED_BODY()

	/** Mission that must be in this state (e.g. Completed_Success) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visibility")
	FName RequiredMissionID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visibility")
	EMissionState RequiredMissionState = EMissionState::Completed_Success;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visibility", meta = (ClampMin = "0"))
	int32 ThreatLevelMin = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visibility", meta = (ClampMin = "0"))
	int32 ThreatLevelMax = 999;

	/** Session time elapsed in seconds - mission becomes visible after this */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visibility", meta = (ClampMin = "0.0"))
	float TimeElapsedSeconds = 0.0f;

	/** All conditions in this struct must pass (AND). Multiple structs = OR between them if we support multiple. */
};

/** Action executed when a mission succeeds or fails (data-driven, no code change to add new missions) */
USTRUCT(BlueprintType)
struct FMissionAction
{
	GENERATED_BODY()

	/** Unhide or activate another mission by ID */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action")
	FName TargetMissionID;

	/** If true, activate the mission; if false, only make it Available */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action")
	bool bActivateMission = false;

	/** Delta to global threat level (e.g. -5 on success, +10 on failure) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action")
	int32 ThreatDelta = 0;

	/** Delta to a specific faction's threat level. FactionID in key. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action")
	TMap<FName, int32> FactionThreatDelta;

	/** Optional: world reaction event name (e.g. AudioEvent_ThreatIncreased). No middleware coupling. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action")
	FName WorldEventName;
};
