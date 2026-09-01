// Shared enums, blackboard key names, and small types for the Enemy AI system.

#pragma once

#include "CoreMinimal.h"
#include "EnemyTypes.generated.h"

/** High-level combat / AI state stored on the Blackboard. */
UENUM(BlueprintType)
enum class EEnemyCombatState : uint8
{
	Idle		UMETA(DisplayName = "Idle"),
	Alert		UMETA(DisplayName = "Alert"),
	Combat		UMETA(DisplayName = "Combat"),
	Retreat		UMETA(DisplayName = "Retreat"),
	Dead		UMETA(DisplayName = "Dead")
};

/** Discrete target selection policy (no weighted scoring). */
UENUM(BlueprintType)
enum class EEnemyTargetPolicy : uint8
{
	Nearest			UMETA(DisplayName = "Nearest"),
	LowestHealth	UMETA(DisplayName = "Lowest Health"),
	HighestAggro	UMETA(DisplayName = "Highest Aggro"),
	StickToCurrent	UMETA(DisplayName = "Stick To Current")
};

/** Movement state for modes that leave normal locomotion (e.g. burrow). */
UENUM(BlueprintType)
enum class EEnemyMovementState : uint8
{
	Normal		UMETA(DisplayName = "Normal"),
	Burrowed	UMETA(DisplayName = "Burrowed"),
	Carrying	UMETA(DisplayName = "Carrying"),
	Carried		UMETA(DisplayName = "Carried")
};

/** Combat flight phase for UFlyingMovementMode (approach → orbit → dive). */
UENUM(BlueprintType)
enum class EEnemyFlyingCombatPhase : uint8
{
	Approach	UMETA(DisplayName = "Approach"),
	Orbit		UMETA(DisplayName = "Orbit"),
	Dive		UMETA(DisplayName = "Dive"),
	PullUp		UMETA(DisplayName = "Pull Up")
};

/** Squad / synergy role tags used by the future coordinator. */
UENUM(BlueprintType)
enum class EEnemySquadRole : uint8
{
	None		UMETA(DisplayName = "None"),
	Carrier		UMETA(DisplayName = "Carrier"),
	Payload		UMETA(DisplayName = "Payload"),
	Support		UMETA(DisplayName = "Support"),
	Assault		UMETA(DisplayName = "Assault")
};

/** Synergy action assigned by squad coordinator (Blackboard). */
UENUM(BlueprintType)
enum class EEnemySynergyAction : uint8
{
	None			UMETA(DisplayName = "None"),
	CarryAlly		UMETA(DisplayName = "Carry Ally"),
	BeCarried		UMETA(DisplayName = "Be Carried"),
	DropPayload		UMETA(DisplayName = "Drop Payload")
};

/** Ability effect category for the lightweight ability system. */
UENUM(BlueprintType)
enum class EEnemyAbilityEffectType : uint8
{
	Damage			UMETA(DisplayName = "Damage"),
	Heal			UMETA(DisplayName = "Heal"),
	Shield			UMETA(DisplayName = "Shield"),
	BuffAlly		UMETA(DisplayName = "Buff Ally"),
	DebuffTarget	UMETA(DisplayName = "Debuff Target"),
	SpawnProjectile	UMETA(DisplayName = "Spawn Projectile"),
	Custom			UMETA(DisplayName = "Custom")
};

/** Ability targeting rule. */
UENUM(BlueprintType)
enum class EEnemyAbilityTargetRule : uint8
{
	CurrentTarget	UMETA(DisplayName = "Current Target"),
	Self			UMETA(DisplayName = "Self"),
	NearestAlly		UMETA(DisplayName = "Nearest Ally"),
	LocationAhead	UMETA(DisplayName = "Location Ahead")
};

/**
 * Canonical Blackboard key names. Create matching keys on your Blackboard asset.
 *
 * For CombatState / MovementState / AssignedRole / SynergyAction:
 * - Prefer Content Browser enums (same names + order as the C++ UENUMs below) as
 *   Blackboard Enum keys, OR use Int keys with the same numeric values.
 * - C++ reads/writes these via SetValueAsEnum / GetValueAsEnum (uint8 ordinals).
 */
struct FEnemyBlackboardKeys
{
	static const FName TargetActor;
	static const FName LastKnownLocation;
	static const FName CombatState;
	static const FName MovementState;
	static const FName NearbyAllyCount;
	static const FName ActiveAbility;
	static const FName SquadId;
	static const FName AssignedRole;
	static const FName SynergyAction;
	static const FName SynergyPartner;
	static const FName AggressionMultiplier;
	static const FName AbilityCooldownScale;
	static const FName HomeLocation;
	static const FName MoveGoal;
};

USTRUCT(BlueprintType)
struct FEnemyMovementParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (ClampMin = "0.0"))
	float MaxSpeed = 800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (ClampMin = "0.0"))
	float Acceleration = 2000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (ClampMin = "0.0"))
	float TurnRateDegPerSec = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (ClampMin = "0.0"))
	float AcceptanceRadius = 150.0f;

	/** Preferred altitude for flying / floating modes (cm above spawn Z or absolute). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float PreferredAltitude = 400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Flocking", meta = (ClampMin = "0.0"))
	float FlockRadius = 800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Flocking", meta = (ClampMin = "0.0"))
	float FlockSeparation = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Burrow", meta = (ClampMin = "0.0"))
	float BurrowRelocateRadius = 2000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Burrow", meta = (ClampMin = "0.0"))
	float BurrowDuration = 1.5f;
};

USTRUCT(BlueprintType)
struct FEnemyPerceptionParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception", meta = (ClampMin = "0.0"))
	float SightRadius = 5000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception", meta = (ClampMin = "0.0"))
	float LoseSightRadius = 6000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float PeripheralVisionAngleDeg = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception")
	bool bDetectEnemies = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception")
	bool bDetectNeutrals = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception")
	bool bDetectFriendlies = false;
};

USTRUCT(BlueprintType)
struct FEnemyAggroParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aggro", meta = (ClampMin = "0.0"))
	float DamageAggroMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aggro", meta = (ClampMin = "0.0"))
	float SightAggroAmount = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aggro", meta = (ClampMin = "0.0"))
	float AggroDecayPerSecond = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aggro", meta = (ClampMin = "0.0"))
	float DropTargetAggroThreshold = 1.0f;

	/** Keep current target until lost / dead even if another has higher aggro. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aggro")
	bool bStickyTarget = true;
};
