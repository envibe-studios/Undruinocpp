#include "AI/EnemyTurretPawn.h"
#include "AI/EnemyDefinition.h"
#include "AI/EnemyHealthComponent.h"
#include "AI/EnemyMovementComponent.h"
#include "AI/StationaryMovementMode.h"
#include "StationaryTurretComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/SkeletalMeshComponent.h"

static USphereComponent* FindSphereByTagOrName(AActor* Owner, FName TagOrName)
{
	if (!Owner || TagOrName.IsNone())
	{
		return nullptr;
	}

	TArray<UActorComponent*> Components;
	Owner->GetComponents(USphereComponent::StaticClass(), Components);
	for (UActorComponent* C : Components)
	{
		if (USphereComponent* SC = Cast<USphereComponent>(C))
		{
			if (SC->ComponentHasTag(TagOrName) || SC->GetFName() == TagOrName)
			{
				return SC;
			}
		}
	}
	return nullptr;
}

AEnemyTurretPawn::AEnemyTurretPawn()
{
	// Emplacements should be hittable by weapon traces / projectiles.
	bDisablePhysicalShipCollision = true;

	if (CapsuleComponent)
	{
		CapsuleComponent->InitCapsuleSize(60.0f, 80.0f);
		CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		CapsuleComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
		CapsuleComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		CapsuleComponent->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
		CapsuleComponent->SetGenerateOverlapEvents(true);
	}

	if (MeshComponent)
	{
		// Visuals live on BP static meshes (yaw/pitch). Keep inherited skeletal mesh hidden.
		MeshComponent->SetHiddenInGame(true);
		MeshComponent->SetVisibility(false);
	}

	// StationaryTurret is authored on the Blueprint (yaw/pitch/muzzle wiring).
	// Do not CreateDefaultSubobject here — that would duplicate BP_Turret's component.
}

void AEnemyTurretPawn::BeginPlay()
{
	CacheTurretComponent();
	ResolveDamageSphere();
	ConfigureTurretForEnemyAI();

	Super::BeginPlay();

	if (HealthComponent)
	{
		HealthComponent->OnEnemyDied.AddDynamic(this, &AEnemyTurretPawn::HandleTurretDied);
	}
}

void AEnemyTurretPawn::CacheTurretComponent()
{
	if (!CachedStationaryTurret)
	{
		CachedStationaryTurret = FindComponentByClass<UStationaryTurretComponent>();
	}
}

void AEnemyTurretPawn::ApplyDefinition(UEnemyDefinition* Definition)
{
	Super::ApplyDefinition(Definition);

	if (!Definition)
	{
		return;
	}

	// Prefer stationary mode when the definition did not specify one.
	if (MovementComponent && !Definition->MovementModeClass)
	{
		MovementComponent->InitializeFromParams(Definition->MovementParams, UStationaryMovementMode::StaticClass());
	}

	ConfigureTurretForEnemyAI();
}

void AEnemyTurretPawn::ConfigureTurretForEnemyAI()
{
	CacheTurretComponent();
	if (!CachedStationaryTurret)
	{
		return;
	}

	CachedStationaryTurret->bUseEnemyAITarget = true;
	CachedStationaryTurret->bPrioritizeShipParts = true;
	CachedStationaryTurret->SetFiringEnabled(true);
}

void AEnemyTurretPawn::ResolveDamageSphere()
{
	if (UActorComponent* Comp = DamageSphereComponent.GetComponent(this))
	{
		ResolvedDamageSphere = Cast<USphereComponent>(Comp);
	}
	if (!ResolvedDamageSphere)
	{
		ResolvedDamageSphere = FindSphereByTagOrName(this, DamageSphereTagOrName);
	}
	if (!ResolvedDamageSphere && bCreateDamageSphere)
	{
		ResolvedDamageSphere = NewObject<USphereComponent>(this, USphereComponent::StaticClass(), TEXT("TurretDamageSphere"));
		if (ResolvedDamageSphere)
		{
			ResolvedDamageSphere->SetupAttachment(GetRootComponent());
			ResolvedDamageSphere->SetSphereRadius(DamageSphereRadius);
			ResolvedDamageSphere->SetCollisionProfileName(DamageSphereCollisionProfile);
			ResolvedDamageSphere->RegisterComponent();
		}
	}
}

void AEnemyTurretPawn::ConfigureNonPhysicalCollision()
{
	// Keep ship-safe defaults for most primitives, but preserve an explicit damage sphere
	// so player weapons can hit the emplacement.
	Super::ConfigureNonPhysicalCollision();

	ResolveDamageSphere();
	if (!ResolvedDamageSphere)
	{
		return;
	}

	ResolvedDamageSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	if (!DamageSphereCollisionProfile.IsNone())
	{
		ResolvedDamageSphere->SetCollisionProfileName(DamageSphereCollisionProfile);
	}
	ResolvedDamageSphere->SetNotifyRigidBodyCollision(false);
	ResolvedDamageSphere->SetGenerateOverlapEvents(true);
	// Still avoid launching the hovercraft on contact.
	ResolvedDamageSphere->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Ignore);
	ResolvedDamageSphere->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Ignore);
}

void AEnemyTurretPawn::HandleTurretDied()
{
	if (CachedStationaryTurret)
	{
		CachedStationaryTurret->SetFiringEnabled(false);
		CachedStationaryTurret->ClearCurrentTarget();
	}
}
