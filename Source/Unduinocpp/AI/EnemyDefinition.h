#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AI/EnemyTypes.h"
#include "EnemyDefinition.generated.h"

class UBehaviorTree;
class UBlackboardData;
class UEnemyAbilityLoadout;
class UEnemyMovementMode;
class AEnemyPawn;

/**
 * Enemy preset — BT, movement mode, abilities, perception, aggro, squad roles.
 * Designers create unique enemy types by authoring definitions + BTs.
 */
UCLASS(BlueprintType)
class UNDUINOCPP_API UEnemyDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Identity")
	FName EnemyId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Identity")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Identity")
	FName FactionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Pawn")
	TSubclassOf<AEnemyPawn> PawnClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|AI")
	TObjectPtr<UBehaviorTree> BehaviorTree;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|AI")
	TObjectPtr<UBlackboardData> BlackboardAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|AI")
	EEnemyTargetPolicy TargetPolicy = EEnemyTargetPolicy::Nearest;

	/**
	 * When true, AEnemyAIController drives movement toward TargetActor each tick.
	 * Stationary emplacements (turrets) should set this false.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|AI")
	bool bAutoPursueTarget = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Movement")
	TSubclassOf<UEnemyMovementMode> MovementModeClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Movement")
	FEnemyMovementParams MovementParams;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Abilities")
	TObjectPtr<UEnemyAbilityLoadout> AbilityLoadout;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Perception")
	FEnemyPerceptionParams PerceptionParams;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Aggro")
	FEnemyAggroParams AggroParams;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Health", meta = (ClampMin = "1.0"))
	float MaxHitpoints = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Squad")
	EEnemySquadRole SquadRole = EEnemySquadRole::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Squad")
	TArray<FName> RoleTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Tags")
	TArray<FName> ActorTags;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("EnemyDefinition"), EnemyId.IsNone() ? GetFName() : EnemyId);
	}
};
