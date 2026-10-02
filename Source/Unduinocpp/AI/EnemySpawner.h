#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemySpawner.generated.h"

class UEnemyDefinition;
class AEnemyPawn;
class UBoxComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemySpawned, AEnemyPawn*, SpawnedEnemy);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemyReturnedToPool, AEnemyPawn*, Enemy);

/**
 * Spawns enemies from definitions with optional simple pooling.
 * Death returns pooled pawns or destroys them; notifies listeners for mission hooks.
 */
UCLASS(Blueprintable)
class UNDUINOCPP_API AEnemySpawner : public AActor
{
	GENERATED_BODY()

public:
	AEnemySpawner();

	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spawner")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spawner")
	TObjectPtr<UBoxComponent> SpawnArea;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	TObjectPtr<UEnemyDefinition> EnemyDefinition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner", meta = (ClampMin = "0"))
	int32 MaxAlive = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner", meta = (ClampMin = "0.0"))
	float SpawnInterval = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	bool bAutoSpawn = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	bool bUsePooling = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner", meta = (ClampMin = "0"))
	int32 PoolSize = 8;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	int32 SquadId = INDEX_NONE;

	UPROPERTY(BlueprintAssignable, Category = "Spawner")
	FOnEnemySpawned OnEnemySpawned;

	UPROPERTY(BlueprintAssignable, Category = "Spawner")
	FOnEnemyReturnedToPool OnEnemyReturnedToPool;

	UFUNCTION(BlueprintCallable, Category = "Spawner")
	AEnemyPawn* SpawnEnemy();

	UFUNCTION(BlueprintCallable, Category = "Spawner")
	void DespawnEnemy(AEnemyPawn* Enemy);

	UFUNCTION(BlueprintPure, Category = "Spawner")
	int32 GetAliveCount() const { return AliveEnemies.Num(); }

protected:
	UFUNCTION()
	void HandleSpawnTimer();

	UFUNCTION()
	void HandleEnemyDied();

	void EnsurePool();
	FVector GetRandomSpawnLocation() const;
	void RegisterAlive(AEnemyPawn* Enemy);
	void UnregisterAlive(AEnemyPawn* Enemy);

	UPROPERTY(Transient)
	TArray<TObjectPtr<AEnemyPawn>> Pool;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AEnemyPawn>> AliveEnemies;

	FTimerHandle SpawnTimerHandle;
};
