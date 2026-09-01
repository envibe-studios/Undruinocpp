#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "AI/EnemyTypes.h"
#include "EnemyAIController.generated.h"

class UBehaviorTree;
class UBlackboardData;
class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UEnemyDefinition;
class AEnemyPawn;
class UAggroComponent;

UCLASS()
class UNDUINOCPP_API AEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	AEnemyAIController();

	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|AI")
	TObjectPtr<UAIPerceptionComponent> PerceptionComp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|AI")
	TObjectPtr<UBehaviorTree> DefaultBehaviorTree;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|AI")
	TObjectPtr<UBlackboardData> DefaultBlackboard;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|AI")
	EEnemyTargetPolicy TargetPolicy = EEnemyTargetPolicy::Nearest;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|AI|LOD", meta = (ClampMin = "0.0"))
	float NearTickInterval = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|AI|LOD", meta = (ClampMin = "0.0"))
	float FarTickInterval = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|AI|LOD", meta = (ClampMin = "0.0"))
	float FarLODDistance = 8000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Debug")
	bool bDrawDebug = false;

	/**
	 * When true, drive UEnemyMovementComponent toward TargetActor each tick.
	 * Needed until the BT uses BTTask_EnemyMoveTo (BP Chase/Roam tasks do not).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|AI")
	bool bAutoPursueTarget = true;

	UFUNCTION(BlueprintCallable, Category = "Enemy|AI")
	void InitializeFromPawn(AEnemyPawn* Enemy);

	UFUNCTION(BlueprintCallable, Category = "Enemy|AI")
	void InitializeFromDefinition(UEnemyDefinition* Definition);

	UFUNCTION(BlueprintCallable, Category = "Enemy|AI")
	void StartBehavior();

	UFUNCTION(BlueprintCallable, Category = "Enemy|AI")
	void StopBehavior();

	UFUNCTION(BlueprintCallable, Category = "Enemy|AI")
	void OnPossessedPawnDied();

	UFUNCTION(BlueprintCallable, Category = "Enemy|AI")
	void SetCombatState(EEnemyCombatState NewState);

	UFUNCTION(BlueprintCallable, Category = "Enemy|AI")
	void SetAggressionMultiplier(float Multiplier);

	UFUNCTION(BlueprintCallable, Category = "Enemy|AI")
	void SetAbilityCooldownScale(float Scale);

	UFUNCTION(BlueprintPure, Category = "Enemy|AI")
	TArray<AActor*> GetPerceivedHostileActors() const;

	UFUNCTION(BlueprintCallable, Category = "Enemy|AI")
	void UpdateTargetFromPolicy();

	UFUNCTION(BlueprintCallable, Category = "Enemy|AI")
	void AssignSynergy(EEnemySynergyAction Action, AActor* Partner, int32 InSquadId);

protected:
	UFUNCTION()
	void HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	void ConfigureSight(const FEnemyPerceptionParams& Params);
	void EnsureBlackboardKeys();
	void UpdateLOD(float DeltaSeconds);
	void UpdateAutoPursuit();
	void DrawDebugOverlay() const;

	UPROPERTY(Transient)
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UPROPERTY(Transient)
	TObjectPtr<UEnemyDefinition> CachedDefinition;

	UPROPERTY(Transient)
	TObjectPtr<AEnemyPawn> CachedEnemy;

	bool bBehaviorRunning = false;
	float LODAccumulator = 0.0f;
};
