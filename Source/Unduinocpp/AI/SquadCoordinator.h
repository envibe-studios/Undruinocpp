#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AI/EnemyTypes.h"
#include "SquadCoordinator.generated.h"

class AEnemyPawn;

/**
 * Future teamwork / synergy layer.
 * Assigns Carry/Drop style orders to eligible Carrier + Payload pairs.
 * Individual BTs only react to SynergyAction / SynergyPartner blackboard keys.
 */
UCLASS(Blueprintable)
class UNDUINOCPP_API ASquadCoordinator : public AActor
{
	GENERATED_BODY()

public:
	ASquadCoordinator();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Squad", meta = (ClampMin = "0.0"))
	float PairSearchRadius = 4000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Squad", meta = (ClampMin = "0.0"))
	float EvaluateInterval = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Squad")
	bool bEnableCarrySynergy = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Squad", meta = (ClampMin = "0.0"))
	float DropDistanceToTarget = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Squad")
	bool bAutoEvaluate = true;

	UFUNCTION(BlueprintCallable, Category = "Squad")
	int32 RegisterEnemy(AEnemyPawn* Enemy, int32 PreferredSquadId = -1);

	UFUNCTION(BlueprintCallable, Category = "Squad")
	void UnregisterEnemy(AEnemyPawn* Enemy);

	UFUNCTION(BlueprintCallable, Category = "Squad")
	void EvaluateSynergies();

	UFUNCTION(BlueprintCallable, Category = "Squad")
	bool TryAssignCarrySynergy(AEnemyPawn* Carrier, AEnemyPawn* Payload);

	UFUNCTION(BlueprintCallable, Category = "Squad")
	void ClearSynergy(AEnemyPawn* Enemy);

protected:
	void TickActiveCarries(float DeltaSeconds);

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<AEnemyPawn>> RegisteredEnemies;

	UPROPERTY(Transient)
	TMap<int32, TWeakObjectPtr<AEnemyPawn>> ActiveCarriers;

	int32 NextSquadId = 1;
	float EvaluateAccumulator = 0.0f;
};
