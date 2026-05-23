// TurretHealthComponent implementation

#include "TurretHealthComponent.h"

#include "Components/SphereComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

static USphereComponent* ResolveSphereComponent(const UActorComponent* OwnerComp, const FComponentReference& Ref)
{
	if (!OwnerComp)
	{
		return nullptr;
	}

	AActor* Owner = OwnerComp->GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	UActorComponent* Comp = Ref.GetComponent(Owner);
	return Cast<USphereComponent>(Comp);
}

static USphereComponent* FindSphereComponentByTagOrName(AActor* Owner, FName TagOrName)
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

UTurretHealthComponent::UTurretHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UTurretHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	CurrentHitpoints = FMath::Clamp(CurrentHitpoints, 0.0f, MaxHitpoints);
	PreviousHitpoints = CurrentHitpoints;

	if (AActor* Owner = GetOwner())
	{
		Owner->OnTakeAnyDamage.AddDynamic(this, &UTurretHealthComponent::HandleOwnerTakeAnyDamage);

		// Prefer an editor-placed sphere component if provided.
		DamageSphere = ResolveSphereComponent(this, DamageSphereComponent);
		if (!DamageSphere)
		{
			DamageSphere = FindSphereComponentByTagOrName(Owner, DamageSphereTagOrName);
		}

		// Fall back to runtime-created sphere if desired.
		if (!DamageSphere && bCreateDamageSphere)
		{
			DamageSphere = NewObject<USphereComponent>(Owner, USphereComponent::StaticClass(), TEXT("TurretDamageSphere"));
			if (DamageSphere)
			{
				DamageSphere->SetupAttachment(Owner->GetRootComponent());
				DamageSphere->SetSphereRadius(DamageSphereRadius);
				DamageSphere->SetCollisionProfileName(DamageSphereCollisionProfile);
				DamageSphere->RegisterComponent();
			}
		}

		// If using an existing sphere, optionally apply defaults (but don't fight authoring if they tuned it).
		if (DamageSphere)
		{
			if (bCreateDamageSphere)
			{
				// Treat these as "defaults": only apply if reasonable.
				if (DamageSphereRadius > 0.0f)
				{
					DamageSphere->SetSphereRadius(DamageSphereRadius);
				}
				if (!DamageSphereCollisionProfile.IsNone())
				{
					DamageSphere->SetCollisionProfileName(DamageSphereCollisionProfile);
				}
			}
		}
	}
}

void UTurretHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UTurretHealthComponent, CurrentHitpoints);
}

void UTurretHealthComponent::OnRep_CurrentHitpoints()
{
	const float Delta = CurrentHitpoints - PreviousHitpoints;
	if (Delta < 0.0f)
	{
		OnTurretDamaged.Broadcast(CurrentHitpoints, -Delta);
	}
	else if (Delta > 0.0f)
	{
		OnTurretHealed.Broadcast(CurrentHitpoints, Delta);
	}

	if (CurrentHitpoints <= 0.0f && !bHasExploded)
	{
		bHasExploded = true;
		OnTurretDestroyed.Broadcast();
		Explode();
	}

	PreviousHitpoints = CurrentHitpoints;
}

float UTurretHealthComponent::ApplyDamage(float DamageAmount, TSubclassOf<UDamageType> DamageTypeClass)
{
	if (!bCanBeDamaged || DamageAmount <= 0.0f || IsDestroyed())
	{
		return CurrentHitpoints;
	}

	float Mult = 1.0f;
	if (DamageTypeMultipliers.Num() > 0 && *DamageTypeClass)
	{
		if (const float* Found = DamageTypeMultipliers.Find(DamageTypeClass))
		{
			Mult = *Found;
		}
	}

	const float EffectiveDamage = FMath::Max(0.0f, DamageAmount * Mult);
	if (EffectiveDamage <= 0.0f)
	{
		return CurrentHitpoints;
	}

	const float Old = CurrentHitpoints;
	CurrentHitpoints = FMath::Max(0.0f, CurrentHitpoints - EffectiveDamage);

	if (CurrentHitpoints != Old)
	{
		OnTurretDamaged.Broadcast(CurrentHitpoints, EffectiveDamage);

		if (CurrentHitpoints <= 0.0f && !bHasExploded)
		{
			bHasExploded = true;
			OnTurretDestroyed.Broadcast();
			Explode();
		}
	}

	PreviousHitpoints = CurrentHitpoints;
	return CurrentHitpoints;
}

float UTurretHealthComponent::Heal(float HealAmount)
{
	if (HealAmount <= 0.0f)
	{
		return CurrentHitpoints;
	}

	const float Old = CurrentHitpoints;
	CurrentHitpoints = FMath::Clamp(CurrentHitpoints + HealAmount, 0.0f, MaxHitpoints);

	const float Delta = CurrentHitpoints - Old;
	if (Delta > 0.0f)
	{
		OnTurretHealed.Broadcast(CurrentHitpoints, Delta);
	}

	PreviousHitpoints = CurrentHitpoints;
	return CurrentHitpoints;
}

void UTurretHealthComponent::SetHitpoints(float NewHitpoints)
{
	const float Old = CurrentHitpoints;
	CurrentHitpoints = FMath::Clamp(NewHitpoints, 0.0f, MaxHitpoints);

	const float Delta = CurrentHitpoints - Old;
	if (Delta < 0.0f)
	{
		OnTurretDamaged.Broadcast(CurrentHitpoints, -Delta);
	}
	else if (Delta > 0.0f)
	{
		OnTurretHealed.Broadcast(CurrentHitpoints, Delta);
	}

	if (CurrentHitpoints <= 0.0f && !bHasExploded)
	{
		bHasExploded = true;
		OnTurretDestroyed.Broadcast();
		Explode();
	}

	PreviousHitpoints = CurrentHitpoints;
}

void UTurretHealthComponent::HandleOwnerTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
	AController* InstigatedBy, AActor* DamageCauser)
{
	if (!bCanBeDamaged || Damage <= 0.0f || IsDestroyed())
	{
		return;
	}

	const TSubclassOf<UDamageType> DamageTypeClass = DamageType ? DamageType->GetClass() : nullptr;
	ApplyDamage(Damage, DamageTypeClass);
}

void UTurretHealthComponent::Explode()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	UWorld* World = Owner->GetWorld();
	if (!World)
	{
		return;
	}

	const FVector Loc = Owner->GetActorLocation();
	const FRotator Rot = Owner->GetActorRotation();

	if (ExplosionFX)
	{
		UGameplayStatics::SpawnEmitterAtLocation(World, ExplosionFX, Loc, Rot, true);
	}
	if (ExplosionSound)
	{
		UGameplayStatics::PlaySoundAtLocation(World, ExplosionSound, Loc);
	}

	if (bDestroyOwnerOnDeath)
	{
		Owner->Destroy();
	}
}

