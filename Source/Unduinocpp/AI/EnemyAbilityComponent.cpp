#include "AI/EnemyAbilityComponent.h"
#include "AI/EnemyAbility.h"
#include "AI/EnemyPawn.h"
#include "AI/EnemyHealthComponent.h"
#include "AI/EnemyAIController.h"
#include "AI/EnemyMovementComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

UEnemyAbilityComponent::UEnemyAbilityComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UEnemyAbilityComponent::BeginPlay()
{
	Super::BeginPlay();
	OwnerEnemy = Cast<AEnemyPawn>(GetOwner());
	RebuildRuntime();
}

void UEnemyAbilityComponent::InitializeFromLoadout(UEnemyAbilityLoadout* InLoadout)
{
	Loadout = InLoadout;
	RebuildRuntime();
}

void UEnemyAbilityComponent::RebuildRuntime()
{
	RuntimeAbilities.Reset();

	TArray<UEnemyAbility*> All;
	if (Loadout)
	{
		for (UEnemyAbility* A : Loadout->Abilities)
		{
			if (A)
			{
				All.Add(A);
			}
		}
	}
	for (UEnemyAbility* A : ExtraAbilities)
	{
		if (A)
		{
			All.AddUnique(A);
		}
	}

	for (UEnemyAbility* Def : All)
	{
		FEnemyAbilityRuntime Rt;
		Rt.Definition = Def;
		Rt.CooldownRemaining = 0.0f;
		Rt.ChargesRemaining = Def->MaxCharges;
		RuntimeAbilities.Add(Rt);
	}
}

void UEnemyAbilityComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	for (FEnemyAbilityRuntime& Rt : RuntimeAbilities)
	{
		if (Rt.CooldownRemaining > 0.0f)
		{
			Rt.CooldownRemaining = FMath::Max(0.0f, Rt.CooldownRemaining - DeltaTime);
		}
	}
}

FEnemyAbilityRuntime* UEnemyAbilityComponent::FindRuntime(FName AbilityId)
{
	for (FEnemyAbilityRuntime& Rt : RuntimeAbilities)
	{
		if (Rt.Definition && Rt.Definition->AbilityId == AbilityId)
		{
			return &Rt;
		}
	}
	return nullptr;
}

const FEnemyAbilityRuntime* UEnemyAbilityComponent::FindRuntime(FName AbilityId) const
{
	for (const FEnemyAbilityRuntime& Rt : RuntimeAbilities)
	{
		if (Rt.Definition && Rt.Definition->AbilityId == AbilityId)
		{
			return &Rt;
		}
	}
	return nullptr;
}

TArray<FName> UEnemyAbilityComponent::GetAbilityIds() const
{
	TArray<FName> Ids;
	for (const FEnemyAbilityRuntime& Rt : RuntimeAbilities)
	{
		if (Rt.Definition)
		{
			Ids.Add(Rt.Definition->AbilityId);
		}
	}
	return Ids;
}

float UEnemyAbilityComponent::GetCooldownRemaining(FName AbilityId) const
{
	if (const FEnemyAbilityRuntime* Rt = FindRuntime(AbilityId))
	{
		return Rt->CooldownRemaining;
	}
	return 0.0f;
}

AActor* UEnemyAbilityComponent::ResolveTarget(const UEnemyAbility* Ability, AActor* OptionalTarget) const
{
	if (!Ability || !OwnerEnemy)
	{
		return nullptr;
	}

	switch (Ability->TargetRule)
	{
	case EEnemyAbilityTargetRule::Self:
		return OwnerEnemy;
	case EEnemyAbilityTargetRule::CurrentTarget:
		if (OptionalTarget)
		{
			return OptionalTarget;
		}
		if (const AEnemyAIController* AIC = Cast<AEnemyAIController>(OwnerEnemy->GetController()))
		{
			if (const UBlackboardComponent* BB = AIC->GetBlackboardComponent())
			{
				return Cast<AActor>(BB->GetValueAsObject(FEnemyBlackboardKeys::TargetActor));
			}
		}
		return nullptr;
	case EEnemyAbilityTargetRule::NearestAlly:
		return OwnerEnemy->FindNearestAlly(Ability->MaxRange);
	case EEnemyAbilityTargetRule::LocationAhead:
		return OptionalTarget;
	default:
		return OptionalTarget;
	}
}

bool UEnemyAbilityComponent::CanActivateAbility(FName AbilityId, AActor* OptionalTarget) const
{
	const FEnemyAbilityRuntime* Rt = FindRuntime(AbilityId);
	if (!Rt || !Rt->Definition || !OwnerEnemy)
	{
		return false;
	}

	if (Rt->CooldownRemaining > 0.0f)
	{
		return false;
	}
	if (Rt->Definition->MaxCharges > 0 && Rt->ChargesRemaining <= 0)
	{
		return false;
	}

	AActor* Target = ResolveTarget(Rt->Definition, OptionalTarget);
	if (Rt->Definition->TargetRule == EEnemyAbilityTargetRule::Self)
	{
		return true;
	}
	if (!Target)
	{
		return false;
	}

	const float Dist = FVector::Dist(OwnerEnemy->GetActorLocation(), Target->GetActorLocation());
	return Dist >= Rt->Definition->MinRange && Dist <= Rt->Definition->MaxRange;
}

bool UEnemyAbilityComponent::ApplyEffect(const UEnemyAbility* Ability, AActor* Target)
{
	if (!Ability || !OwnerEnemy)
	{
		return false;
	}

	UWorld* World = OwnerEnemy->GetWorld();
	if (!World)
	{
		return false;
	}

	switch (Ability->EffectType)
	{
	case EEnemyAbilityEffectType::SpawnProjectile:
	{
		if (!Ability->ProjectileClass)
		{
			// Hitscan fallback damage
			if (Target)
			{
				UGameplayStatics::ApplyDamage(Target, Ability->Magnitude, OwnerEnemy->GetController(), OwnerEnemy, nullptr);
			}
			return true;
		}

		const FVector SpawnLoc = OwnerEnemy->GetActorLocation() + OwnerEnemy->GetActorForwardVector() * 80.0f;
		FVector Aim = OwnerEnemy->GetActorForwardVector();
		if (Target)
		{
			Aim = (Target->GetActorLocation() - SpawnLoc).GetSafeNormal();
		}
		const FRotator SpawnRot = Aim.Rotation();
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = OwnerEnemy;
		SpawnParams.Instigator = OwnerEnemy;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		if (AActor* Proj = World->SpawnActor<AActor>(Ability->ProjectileClass, SpawnLoc, SpawnRot, SpawnParams))
		{
			if (UProjectileMovementComponent* PMC = Proj->FindComponentByClass<UProjectileMovementComponent>())
			{
				PMC->Velocity = Aim * Ability->ProjectileSpeed;
				PMC->InitialSpeed = Ability->ProjectileSpeed;
			}
			return true;
		}
		return false;
	}
	case EEnemyAbilityEffectType::Damage:
		if (Target)
		{
			UGameplayStatics::ApplyDamage(Target, Ability->Magnitude, OwnerEnemy->GetController(), OwnerEnemy, nullptr);
			return true;
		}
		return false;
	case EEnemyAbilityEffectType::Heal:
	{
		AActor* HealTarget = Target ? Target : OwnerEnemy.Get();
		if (UEnemyHealthComponent* Health = HealTarget->FindComponentByClass<UEnemyHealthComponent>())
		{
			Health->Heal(Ability->Magnitude);
			return true;
		}
		return false;
	}
	case EEnemyAbilityEffectType::Shield:
	{
		AActor* ShieldTarget = Target ? Target : OwnerEnemy.Get();
		if (UEnemyHealthComponent* Health = ShieldTarget->FindComponentByClass<UEnemyHealthComponent>())
		{
			Health->AddShield(Ability->Magnitude);
			return true;
		}
		return false;
	}
	case EEnemyAbilityEffectType::BuffAlly:
	{
		AActor* Ally = Target ? Target : OwnerEnemy->FindNearestAlly(Ability->MaxRange);
		if (Ally)
		{
			if (UEnemyMovementComponent* Move = Ally->FindComponentByClass<UEnemyMovementComponent>())
			{
				Move->SetSpeedMultiplier(1.0f + Ability->Magnitude * 0.01f);
			}
			return true;
		}
		return false;
	}
	case EEnemyAbilityEffectType::DebuffTarget:
		if (Target)
		{
			if (UEnemyMovementComponent* Move = Target->FindComponentByClass<UEnemyMovementComponent>())
			{
				Move->SetSpeedMultiplier(FMath::Max(0.2f, 1.0f - Ability->Magnitude * 0.01f));
			}
			return true;
		}
		return false;
	case EEnemyAbilityEffectType::Custom:
		return true;
	default:
		return false;
	}
}

bool UEnemyAbilityComponent::ActivateAbility(FName AbilityId, AActor* OptionalTarget)
{
	FEnemyAbilityRuntime* Rt = FindRuntime(AbilityId);
	if (!Rt || !Rt->Definition)
	{
		OnAbilityFailed.Broadcast(AbilityId);
		return false;
	}

	if (!CanActivateAbility(AbilityId, OptionalTarget))
	{
		OnAbilityFailed.Broadcast(AbilityId);
		return false;
	}

	AActor* Target = ResolveTarget(Rt->Definition, OptionalTarget);
	if (!ApplyEffect(Rt->Definition, Target))
	{
		OnAbilityFailed.Broadcast(AbilityId);
		return false;
	}

	Rt->CooldownRemaining = Rt->Definition->CooldownSeconds * CooldownScale;
	if (Rt->Definition->MaxCharges > 0)
	{
		Rt->ChargesRemaining = FMath::Max(0, Rt->ChargesRemaining - 1);
	}

	if (AEnemyAIController* AIC = Cast<AEnemyAIController>(OwnerEnemy->GetController()))
	{
		if (UBlackboardComponent* BB = AIC->GetBlackboardComponent())
		{
			BB->SetValueAsName(FEnemyBlackboardKeys::ActiveAbility, AbilityId);
		}
	}

	OnAbilityActivated.Broadcast(AbilityId, Target);
	return true;
}
