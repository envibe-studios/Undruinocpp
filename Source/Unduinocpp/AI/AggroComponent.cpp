#include "AI/AggroComponent.h"
#include "AI/EnemyHealthComponent.h"
#include "AI/EnemyPawn.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"

UAggroComponent::UAggroComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UAggroComponent::BeginPlay()
{
	Super::BeginPlay();
	if (AActor* Owner = GetOwner())
	{
		Owner->OnTakeAnyDamage.AddDynamic(this, &UAggroComponent::HandleOwnerTakeAnyDamage);
	}
}

void UAggroComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const float Decay = AggroParams.AggroDecayPerSecond * DeltaTime;
	for (int32 i = AggroTable.Num() - 1; i >= 0; --i)
	{
		FAggroEntry& Entry = AggroTable[i];
		if (!Entry.Target.IsValid())
		{
			AggroTable.RemoveAtSwap(i);
			continue;
		}
		Entry.Aggro = FMath::Max(0.0f, Entry.Aggro - Decay);
		if (Entry.Aggro < AggroParams.DropTargetAggroThreshold)
		{
			AggroTable.RemoveAtSwap(i);
		}
	}
}

void UAggroComponent::AddAggro(AActor* Target, float Amount)
{
	if (!Target || Amount <= 0.0f)
	{
		return;
	}

	for (FAggroEntry& Entry : AggroTable)
	{
		if (Entry.Target.Get() == Target)
		{
			Entry.Aggro += Amount;
			return;
		}
	}

	FAggroEntry NewEntry;
	NewEntry.Target = Target;
	NewEntry.Aggro = Amount;
	AggroTable.Add(NewEntry);
}

void UAggroComponent::ClearAggro(AActor* Target)
{
	AggroTable.RemoveAll([Target](const FAggroEntry& E) { return E.Target.Get() == Target; });
}

void UAggroComponent::ClearAllAggro()
{
	AggroTable.Reset();
}

float UAggroComponent::GetAggro(AActor* Target) const
{
	for (const FAggroEntry& Entry : AggroTable)
	{
		if (Entry.Target.Get() == Target)
		{
			return Entry.Aggro;
		}
	}
	return 0.0f;
}

AActor* UAggroComponent::GetHighestAggroTarget() const
{
	AActor* Best = nullptr;
	float BestAggro = -1.0f;
	for (const FAggroEntry& Entry : AggroTable)
	{
		if (AActor* T = Entry.Target.Get())
		{
			if (Entry.Aggro > BestAggro)
			{
				BestAggro = Entry.Aggro;
				Best = T;
			}
		}
	}
	return Best;
}

void UAggroComponent::NotifyDamagedBy(AActor* DamageCauser, float DamageAmount)
{
	AActor* AggroSource = DamageCauser;
	if (!AggroSource)
	{
		return;
	}
	if (AController* C = Cast<AController>(DamageCauser))
	{
		AggroSource = C->GetPawn();
	}
	else if (DamageCauser->GetInstigator())
	{
		AggroSource = DamageCauser->GetInstigator();
	}
	else if (DamageCauser->GetOwner())
	{
		AggroSource = DamageCauser->GetOwner();
	}
	AddAggro(AggroSource, DamageAmount * AggroParams.DamageAggroMultiplier);
}

void UAggroComponent::HandleOwnerTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
	AController* InstigatedBy, AActor* DamageCauser)
{
	AActor* Source = DamageCauser;
	if (InstigatedBy && InstigatedBy->GetPawn())
	{
		Source = InstigatedBy->GetPawn();
	}
	NotifyDamagedBy(Source, Damage);
}

void UAggroComponent::NotifySightOn(AActor* SeenActor)
{
	AddAggro(SeenActor, AggroParams.SightAggroAmount);
}

AActor* UAggroComponent::SelectTarget(EEnemyTargetPolicy Policy, AActor* CurrentTarget, const TArray<AActor*>& PerceivedActors) const
{
	auto IsValidTarget = [](AActor* A) -> bool
	{
		return A && !A->IsActorBeingDestroyed() && A->GetClass();
	};

	if (Policy == EEnemyTargetPolicy::StickToCurrent && IsValidTarget(CurrentTarget))
	{
		return CurrentTarget;
	}

	if (AggroParams.bStickyTarget && IsValidTarget(CurrentTarget) && Policy != EEnemyTargetPolicy::HighestAggro)
	{
		// Keep current if still perceived
		if (PerceivedActors.Contains(CurrentTarget))
		{
			return CurrentTarget;
		}
	}

	AActor* Owner = GetOwner();
	const FVector Origin = Owner ? Owner->GetActorLocation() : FVector::ZeroVector;

	switch (Policy)
	{
	case EEnemyTargetPolicy::HighestAggro:
	{
		AActor* Best = GetHighestAggroTarget();
		if (IsValidTarget(Best))
		{
			return Best;
		}
		// fall through to nearest
	}
	case EEnemyTargetPolicy::Nearest:
	{
		AActor* Best = nullptr;
		float BestDistSq = TNumericLimits<float>::Max();
		for (AActor* Candidate : PerceivedActors)
		{
			if (!IsValidTarget(Candidate))
			{
				continue;
			}
			const float DistSq = FVector::DistSquared(Origin, Candidate->GetActorLocation());
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				Best = Candidate;
			}
		}
		return Best;
	}
	case EEnemyTargetPolicy::LowestHealth:
	{
		AActor* Best = nullptr;
		float BestHealth = TNumericLimits<float>::Max();
		for (AActor* Candidate : PerceivedActors)
		{
			if (!IsValidTarget(Candidate))
			{
				continue;
			}
			float Health = TNumericLimits<float>::Max();
			if (UEnemyHealthComponent* EH = Candidate->FindComponentByClass<UEnemyHealthComponent>())
			{
				Health = EH->CurrentHitpoints;
			}
			else
			{
				// No health component — use distance as a stand-in so we still pick someone.
				Health = FVector::Dist(Origin, Candidate->GetActorLocation());
			}
			if (Health < BestHealth)
			{
				BestHealth = Health;
				Best = Candidate;
			}
		}
		return Best;
	}
	default:
		return CurrentTarget;
	}
}
