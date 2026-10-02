#include "AI/EnemyHealthComponent.h"

#include "GameFramework/Actor.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

UEnemyHealthComponent::UEnemyHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UEnemyHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	CurrentHitpoints = FMath::Clamp(CurrentHitpoints, 0.0f, MaxHitpoints);
	ShieldHitpoints = FMath::Clamp(ShieldHitpoints, 0.0f, MaxShieldHitpoints);
	PreviousHitpoints = CurrentHitpoints;
	PreviousShield = ShieldHitpoints;

	if (AActor* Owner = GetOwner())
	{
		Owner->OnTakeAnyDamage.AddDynamic(this, &UEnemyHealthComponent::HandleOwnerTakeAnyDamage);
	}
}

void UEnemyHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UEnemyHealthComponent, CurrentHitpoints);
	DOREPLIFETIME(UEnemyHealthComponent, ShieldHitpoints);
}

void UEnemyHealthComponent::OnRep_CurrentHitpoints()
{
	const float Delta = CurrentHitpoints - PreviousHitpoints;
	if (Delta < 0.0f)
	{
		OnEnemyDamaged.Broadcast(CurrentHitpoints, -Delta);
	}
	else if (Delta > 0.0f)
	{
		OnEnemyHealed.Broadcast(CurrentHitpoints, Delta);
	}

	if (CurrentHitpoints <= 0.0f && !bHasDied)
	{
		HandleDeath();
	}

	PreviousHitpoints = CurrentHitpoints;
}

void UEnemyHealthComponent::OnRep_Shield()
{
	OnEnemyShieldChanged.Broadcast(ShieldHitpoints);
	PreviousShield = ShieldHitpoints;
}

float UEnemyHealthComponent::ApplyDamage(float DamageAmount, AController* InstigatedBy, AActor* DamageCauser, TSubclassOf<UDamageType> DamageTypeClass)
{
	if (!bCanBeDamaged || DamageAmount <= 0.0f || IsDead())
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

	float Remaining = FMath::Max(0.0f, DamageAmount * Mult);

	if (ShieldHitpoints > 0.0f && Remaining > 0.0f)
	{
		const float Absorbed = FMath::Min(ShieldHitpoints, Remaining);
		ShieldHitpoints -= Absorbed;
		Remaining -= Absorbed;
		OnEnemyShieldChanged.Broadcast(ShieldHitpoints);
		PreviousShield = ShieldHitpoints;
	}

	if (Remaining <= 0.0f)
	{
		return CurrentHitpoints;
	}

	const float Old = CurrentHitpoints;
	CurrentHitpoints = FMath::Max(0.0f, CurrentHitpoints - Remaining);

	if (CurrentHitpoints != Old)
	{
		OnEnemyDamaged.Broadcast(CurrentHitpoints, Remaining);
		if (CurrentHitpoints <= 0.0f && !bHasDied)
		{
			HandleDeath();
		}
	}

	PreviousHitpoints = CurrentHitpoints;
	return CurrentHitpoints;
}

float UEnemyHealthComponent::Heal(float HealAmount)
{
	if (HealAmount <= 0.0f || IsDead())
	{
		return CurrentHitpoints;
	}

	const float Old = CurrentHitpoints;
	CurrentHitpoints = FMath::Clamp(CurrentHitpoints + HealAmount, 0.0f, MaxHitpoints);
	const float Delta = CurrentHitpoints - Old;
	if (Delta > 0.0f)
	{
		OnEnemyHealed.Broadcast(CurrentHitpoints, Delta);
	}
	PreviousHitpoints = CurrentHitpoints;
	return CurrentHitpoints;
}

void UEnemyHealthComponent::AddShield(float Amount)
{
	if (Amount <= 0.0f || IsDead())
	{
		return;
	}

	const float Cap = MaxShieldHitpoints > 0.0f ? MaxShieldHitpoints : ShieldHitpoints + Amount;
	ShieldHitpoints = FMath::Clamp(ShieldHitpoints + Amount, 0.0f, Cap);
	if (MaxShieldHitpoints < ShieldHitpoints)
	{
		MaxShieldHitpoints = ShieldHitpoints;
	}
	OnEnemyShieldChanged.Broadcast(ShieldHitpoints);
	PreviousShield = ShieldHitpoints;
}

void UEnemyHealthComponent::SetHitpoints(float NewHitpoints)
{
	const float Old = CurrentHitpoints;
	CurrentHitpoints = FMath::Clamp(NewHitpoints, 0.0f, MaxHitpoints);
	const float Delta = CurrentHitpoints - Old;
	if (Delta < 0.0f)
	{
		OnEnemyDamaged.Broadcast(CurrentHitpoints, -Delta);
	}
	else if (Delta > 0.0f)
	{
		OnEnemyHealed.Broadcast(CurrentHitpoints, Delta);
	}
	if (CurrentHitpoints <= 0.0f && !bHasDied)
	{
		HandleDeath();
	}
	PreviousHitpoints = CurrentHitpoints;
}

void UEnemyHealthComponent::HandleOwnerTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
	AController* InstigatedBy, AActor* DamageCauser)
{
	const TSubclassOf<UDamageType> DamageTypeClass = DamageType ? DamageType->GetClass() : nullptr;
	ApplyDamage(Damage, InstigatedBy, DamageCauser, DamageTypeClass);
}

void UEnemyHealthComponent::HandleDeath()
{
	if (bHasDied)
	{
		return;
	}
	bHasDied = true;
	OnEnemyDied.Broadcast();

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	UWorld* World = Owner->GetWorld();
	if (World)
	{
		const FVector Loc = Owner->GetActorLocation();
		const FRotator Rot = Owner->GetActorRotation();
		if (DeathFX)
		{
			UGameplayStatics::SpawnEmitterAtLocation(World, DeathFX, Loc, Rot, true);
		}
		if (DeathSound)
		{
			UGameplayStatics::PlaySoundAtLocation(World, DeathSound, Loc);
		}
	}

	if (bDestroyOwnerOnDeath)
	{
		Owner->SetLifeSpan(0.05f);
	}
}
