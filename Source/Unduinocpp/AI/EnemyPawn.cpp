#include "AI/EnemyPawn.h"
#include "AI/EnemyDefinition.h"
#include "AI/EnemyHealthComponent.h"
#include "AI/EnemyMovementComponent.h"
#include "AI/EnemyAbilityComponent.h"
#include "AI/AggroComponent.h"
#include "AI/EnemyAIController.h"
#include "AI/FlyingMovementMode.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

AEnemyPawn::AEnemyPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicatingMovement(true);
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = AEnemyAIController::StaticClass();

	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComponent"));
	CapsuleComponent->InitCapsuleSize(42.0f, 60.0f);
	CapsuleComponent->SetCollisionProfileName(TEXT("Pawn"));
	// Query-only: still detectable, but kinematic moves won't batter physics vehicles.
	CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CapsuleComponent->SetNotifyRigidBodyCollision(false);
	RootComponent = CapsuleComponent;

	MeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	HealthComponent = CreateDefaultSubobject<UEnemyHealthComponent>(TEXT("HealthComponent"));
	MovementComponent = CreateDefaultSubobject<UEnemyMovementComponent>(TEXT("EnemyMovementComponent"));
	AbilityComponent = CreateDefaultSubobject<UEnemyAbilityComponent>(TEXT("AbilityComponent"));
	AggroComponent = CreateDefaultSubobject<UAggroComponent>(TEXT("AggroComponent"));
}

void AEnemyPawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AEnemyPawn, SquadId);
	DOREPLIFETIME(AEnemyPawn, SquadRole);
}

void AEnemyPawn::BeginPlay()
{
	Super::BeginPlay();
	HomeLocation = GetActorLocation();
	ApplyDefaultTags();
	ConfigureNonPhysicalCollision();

	if (EnemyDefinition)
	{
		ApplyDefinition(EnemyDefinition);
	}

	if (HealthComponent)
	{
		HealthComponent->OnEnemyDied.AddDynamic(this, &AEnemyPawn::HandleDied);
	}
}

void AEnemyPawn::ConfigureNonPhysicalCollision()
{
	if (!bDisablePhysicalShipCollision)
	{
		return;
	}

	TArray<UPrimitiveComponent*> Primitives;
	GetComponents<UPrimitiveComponent>(Primitives);
	for (UPrimitiveComponent* Prim : Primitives)
	{
		if (!Prim)
		{
			continue;
		}

		// Proxy meshes must not batter the physics hovercraft.
		if (Prim != CapsuleComponent)
		{
			Prim->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Prim->SetNotifyRigidBodyCollision(false);
			continue;
		}

		// Query-only: sweeps/traces still work (world/ground), but no rigid contacts
		// that can launch or flip a simulating vehicle.
		Prim->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Prim->SetNotifyRigidBodyCollision(false);
		Prim->SetGenerateOverlapEvents(true);
		Prim->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Ignore);
		Prim->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Ignore);
		Prim->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Ignore);
		Prim->SetCollisionResponseToChannel(ECC_Destructible, ECR_Ignore);
		Prim->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	}
}

void AEnemyPawn::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	if (AEnemyAIController* AIC = Cast<AEnemyAIController>(NewController))
	{
		AIC->InitializeFromPawn(this);
	}
}

void AEnemyPawn::ApplyDefaultTags()
{
	Tags.AddUnique(FName(TEXT("Targetable")));
	Tags.AddUnique(FName(TEXT("Enemy")));
}

void AEnemyPawn::ApplyDefinition(UEnemyDefinition* Definition)
{
	if (!Definition)
	{
		return;
	}

	EnemyDefinition = Definition;

	if (HealthComponent)
	{
		HealthComponent->MaxHitpoints = Definition->MaxHitpoints;
		HealthComponent->CurrentHitpoints = Definition->MaxHitpoints;
	}

	if (MovementComponent)
	{
		const TSubclassOf<UEnemyMovementMode> ModeClass = Definition->MovementModeClass
			? Definition->MovementModeClass
			: TSubclassOf<UEnemyMovementMode>(UFlyingMovementMode::StaticClass());
		MovementComponent->InitializeFromParams(Definition->MovementParams, ModeClass);
	}

	if (AbilityComponent && Definition->AbilityLoadout)
	{
		AbilityComponent->InitializeFromLoadout(Definition->AbilityLoadout);
	}

	if (AggroComponent)
	{
		AggroComponent->AggroParams = Definition->AggroParams;
	}

	SquadRole = Definition->SquadRole;

	for (const FName& Tag : Definition->ActorTags)
	{
		Tags.AddUnique(Tag);
	}

	if (AEnemyAIController* AIC = Cast<AEnemyAIController>(GetController()))
	{
		AIC->InitializeFromDefinition(Definition);
	}
}

AActor* AEnemyPawn::FindNearestAlly(float MaxRange) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(World, AEnemyPawn::StaticClass(), Found);

	AActor* Best = nullptr;
	float BestDistSq = FMath::Square(MaxRange);
	const FVector Origin = GetActorLocation();

	for (AActor* Actor : Found)
	{
		if (Actor == this)
		{
			continue;
		}
		const float DistSq = FVector::DistSquared(Origin, Actor->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Best = Actor;
		}
	}
	return Best;
}

void AEnemyPawn::SetSquadId(int32 InSquadId)
{
	SquadId = InSquadId;
}

float AEnemyPawn::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// UEnemyHealthComponent listens to OnTakeAnyDamage (broadcast by Super).
	return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
}

void AEnemyPawn::HandleDied()
{
	if (AEnemyAIController* AIC = Cast<AEnemyAIController>(GetController()))
	{
		AIC->OnPossessedPawnDied();
	}
}
