#include "AI/EnemyPawn.h"
#include "AI/EnemyDefinition.h"
#include "AI/EnemyHealthComponent.h"
#include "AI/EnemyMovementComponent.h"
#include "AI/EnemyAbilityComponent.h"
#include "AI/AggroComponent.h"
#include "AI/EnemyAIController.h"
#include "AI/FlyingMovementMode.h"
#include "HoverMovementComponent.h"
#include "HoverThrusterComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/BoxComponent.h"
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
	if (bHoverRaiderPhysicsConfigured)
	{
		return;
	}

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
	if (Definition->bEnableFlankingRaiderBehavior)
	{
		ConfigureHoverRaiderPhysics();
	}

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

void AEnemyPawn::ConfigureHoverRaiderPhysics()
{
	if (bHoverRaiderPhysicsConfigured || !CapsuleComponent || !RootComponent)
	{
		return;
	}

	// Keep the hover supports under the hull, matching BP_Hovercraft's PhysicsRoot box.
	SetActorScale3D(GetActorScale3D() * 2.0f);
	const FTransform PreviousRootTransform = RootComponent->GetComponentTransform();
	HoverRaiderPhysicsRoot = NewObject<UBoxComponent>(this, TEXT("RaiderPhysicsRoot"));
	AddInstanceComponent(HoverRaiderPhysicsRoot);
	HoverRaiderPhysicsRoot->SetBoxExtent(FVector(137.5f, 50.0f, 50.0f));
	HoverRaiderPhysicsRoot->SetCollisionProfileName(TEXT("PhysicsActor"));
	HoverRaiderPhysicsRoot->SetWorldTransform(PreviousRootTransform);
	SetRootComponent(HoverRaiderPhysicsRoot);
	HoverRaiderPhysicsRoot->RegisterComponent();
	CapsuleComponent->AttachToComponent(HoverRaiderPhysicsRoot, FAttachmentTransformRules::KeepWorldTransform);
	CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// The flanking raider uses the same four ground-tracing hover springs as BP_Hovercraft.
	HoverRaiderMovement = NewObject<UHoverMovementComponent>(this, TEXT("RaiderHoverMovement"));
	AddInstanceComponent(HoverRaiderMovement);
	HoverRaiderMovement->MaxForwardThrust = 1200000.0f;
	HoverRaiderMovement->MaxBackwardThrust = 600000.0f;
	HoverRaiderMovement->ThrustAcceleration = 12000.0f;
	HoverRaiderMovement->ThrustDeceleration = 10000.0f;
	HoverRaiderMovement->LinearDrag = 800.0f;
	HoverRaiderMovement->MaxTurnTorque = 15000000.0f;
	HoverRaiderMovement->bEnableStrafe = false;
	HoverRaiderMovement->bEnableBoost = false;

	const TCHAR* ThrusterNames[] = { TEXT("Thruster_FL"), TEXT("Thruster_FR"), TEXT("Thruster_RL"), TEXT("Thruster_RR") };
	const FVector ThrusterLocations[] = {
		FVector(135.0f, -50.0f, -50.0f), FVector(135.0f, 50.0f, -50.0f),
		FVector(-135.0f, -50.0f, -50.0f), FVector(-135.0f, 50.0f, -50.0f)
	};
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(ThrusterNames); ++Index)
	{
		UHoverThrusterComponent* Thruster = NewObject<UHoverThrusterComponent>(this, ThrusterNames[Index]);
		AddInstanceComponent(Thruster);
		Thruster->SetupAttachment(RootComponent);
		Thruster->SetRelativeLocation(ThrusterLocations[Index]);
		Thruster->HoverHeight = 75.0f;
		Thruster->MaxHoverForce = 500000.0f;
		Thruster->HoverStiffness = 5000.0f;
		Thruster->HoverDamping = 1000.0f;
		Thruster->RegisterComponent();
		HoverRaiderMovement->RegisterThruster(Thruster);
		HoverRaiderThrusters.Add(Thruster);
	}
	// Register the movement component after its supports are configured.
	HoverRaiderMovement->RegisterComponent();

	HoverRaiderPhysicsRoot->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	HoverRaiderPhysicsRoot->SetNotifyRigidBodyCollision(true);
	HoverRaiderPhysicsRoot->SetSimulatePhysics(true);
	HoverRaiderPhysicsRoot->SetEnableGravity(true);
	HoverRaiderPhysicsRoot->SetLinearDamping(0.5f);
	HoverRaiderPhysicsRoot->SetAngularDamping(2.0f);
	HoverRaiderPhysicsRoot->SetMassOverrideInKg(NAME_None, 800.0f, true);
	HoverRaiderPhysicsRoot->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	HoverRaiderPhysicsRoot->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	HoverRaiderPhysicsRoot->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	HoverRaiderPhysicsRoot->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Block);
	HoverRaiderPhysicsRoot->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Block);
	bHoverRaiderPhysicsConfigured = true;
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
