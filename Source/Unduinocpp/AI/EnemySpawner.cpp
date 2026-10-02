#include "AI/EnemySpawner.h"
#include "AI/EnemyDefinition.h"
#include "AI/EnemyPawn.h"
#include "AI/EnemyHealthComponent.h"
#include "AI/EnemyAIController.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

AEnemySpawner::AEnemySpawner()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	SpawnArea = CreateDefaultSubobject<UBoxComponent>(TEXT("SpawnArea"));
	SpawnArea->SetupAttachment(RootComponent);
	SpawnArea->SetBoxExtent(FVector(500.0f, 500.0f, 100.0f));
	SpawnArea->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AEnemySpawner::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		return;
	}

	if (bUsePooling)
	{
		EnsurePool();
	}

	if (bAutoSpawn && SpawnInterval > 0.0f)
	{
		GetWorldTimerManager().SetTimer(SpawnTimerHandle, this, &AEnemySpawner::HandleSpawnTimer, SpawnInterval, true, 0.5f);
	}
	else if (bAutoSpawn)
	{
		SpawnEnemy();
	}
}

void AEnemySpawner::EnsurePool()
{
	if (!EnemyDefinition)
	{
		return;
	}

	UClass* PawnClass = EnemyDefinition->PawnClass
		? *EnemyDefinition->PawnClass
		: AEnemyPawn::StaticClass();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	while (Pool.Num() < PoolSize)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Params.Owner = this;
		AEnemyPawn* Pawn = World->SpawnActor<AEnemyPawn>(PawnClass, GetActorLocation(), GetActorRotation(), Params);
		if (!Pawn)
		{
			break;
		}
		Pawn->SetActorHiddenInGame(true);
		Pawn->SetActorEnableCollision(false);
		Pawn->SetActorTickEnabled(false);
		if (AController* C = Pawn->GetController())
		{
			C->UnPossess();
		}
		Pool.Add(Pawn);
	}
}

FVector AEnemySpawner::GetRandomSpawnLocation() const
{
	if (!SpawnArea)
	{
		return GetActorLocation();
	}

	const FVector Extent = SpawnArea->GetScaledBoxExtent();
	const FVector Origin = SpawnArea->GetComponentLocation();
	const FVector Offset(
		FMath::FRandRange(-Extent.X, Extent.X),
		FMath::FRandRange(-Extent.Y, Extent.Y),
		FMath::FRandRange(-Extent.Z, Extent.Z));
	return Origin + Offset;
}

AEnemyPawn* AEnemySpawner::SpawnEnemy()
{
	if (!HasAuthority() || !EnemyDefinition)
	{
		return nullptr;
	}

	if (AliveEnemies.Num() >= MaxAlive)
	{
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	AEnemyPawn* Pawn = nullptr;
	const FVector Loc = GetRandomSpawnLocation();
	const FRotator Rot = GetActorRotation();

	if (bUsePooling)
	{
		EnsurePool();
		for (int32 i = 0; i < Pool.Num(); ++i)
		{
			if (Pool[i] && Pool[i]->IsHidden())
			{
				Pawn = Pool[i];
				Pool.RemoveAtSwap(i);
				break;
			}
		}
	}

	if (!Pawn)
	{
		UClass* PawnClass = EnemyDefinition->PawnClass ? *EnemyDefinition->PawnClass : AEnemyPawn::StaticClass();
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		Params.Owner = this;
		Pawn = World->SpawnActor<AEnemyPawn>(PawnClass, Loc, Rot, Params);
	}
	else
	{
		Pawn->SetActorLocationAndRotation(Loc, Rot, false, nullptr, ETeleportType::ResetPhysics);
		Pawn->SetActorHiddenInGame(false);
		Pawn->SetActorEnableCollision(true);
		Pawn->SetActorTickEnabled(true);
	}

	if (!Pawn)
	{
		return nullptr;
	}

	if (Pawn->HealthComponent)
	{
		Pawn->HealthComponent->bDestroyOwnerOnDeath = !bUsePooling;
		Pawn->HealthComponent->MaxHitpoints = EnemyDefinition->MaxHitpoints;
		Pawn->HealthComponent->CurrentHitpoints = EnemyDefinition->MaxHitpoints;
		Pawn->HealthComponent->OnEnemyDied.AddUniqueDynamic(this, &AEnemySpawner::HandleEnemyDied);
	}

	Pawn->ApplyDefinition(EnemyDefinition);
	if (SquadId != INDEX_NONE)
	{
		Pawn->SetSquadId(SquadId);
	}

	// Ensure AI possession
	if (!Pawn->GetController())
	{
		Pawn->SpawnDefaultController();
	}
	if (AEnemyAIController* AIC = Cast<AEnemyAIController>(Pawn->GetController()))
	{
		AIC->InitializeFromDefinition(EnemyDefinition);
		AIC->StartBehavior();
	}

	RegisterAlive(Pawn);
	OnEnemySpawned.Broadcast(Pawn);
	return Pawn;
}

void AEnemySpawner::DespawnEnemy(AEnemyPawn* Enemy)
{
	if (!Enemy)
	{
		return;
	}

	UnregisterAlive(Enemy);

	if (AEnemyAIController* AIC = Cast<AEnemyAIController>(Enemy->GetController()))
	{
		AIC->StopBehavior();
		AIC->UnPossess();
	}

	if (bUsePooling)
	{
		Enemy->SetActorHiddenInGame(true);
		Enemy->SetActorEnableCollision(false);
		Enemy->SetActorTickEnabled(false);
		Pool.Add(Enemy);
		OnEnemyReturnedToPool.Broadcast(Enemy);
	}
	else
	{
		Enemy->Destroy();
	}
}

void AEnemySpawner::HandleSpawnTimer()
{
	SpawnEnemy();
}

void AEnemySpawner::HandleEnemyDied()
{
	// Find which alive enemy is dead and recycle (or just unregister if not pooling).
	for (int32 i = AliveEnemies.Num() - 1; i >= 0; --i)
	{
		AEnemyPawn* Enemy = AliveEnemies[i];
		if (!Enemy)
		{
			AliveEnemies.RemoveAtSwap(i);
			continue;
		}
		if (Enemy->HealthComponent && Enemy->HealthComponent->IsDead())
		{
			Enemy->HealthComponent->OnEnemyDied.RemoveDynamic(this, &AEnemySpawner::HandleEnemyDied);
			if (bUsePooling)
			{
				DespawnEnemy(Enemy);
			}
			else
			{
				UnregisterAlive(Enemy);
			}
		}
	}
}

void AEnemySpawner::RegisterAlive(AEnemyPawn* Enemy)
{
	AliveEnemies.AddUnique(Enemy);
}

void AEnemySpawner::UnregisterAlive(AEnemyPawn* Enemy)
{
	AliveEnemies.Remove(Enemy);
}
