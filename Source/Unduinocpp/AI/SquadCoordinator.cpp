#include "AI/SquadCoordinator.h"
#include "AI/EnemyPawn.h"
#include "AI/EnemyDefinition.h"
#include "AI/EnemyAIController.h"
#include "AI/EnemyMovementComponent.h"
#include "AI/EnemyMovementMode.h"
#include "AI/EnemyHealthComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AI/EnemyTypes.h"

ASquadCoordinator::ASquadCoordinator()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
}

void ASquadCoordinator::BeginPlay()
{
	Super::BeginPlay();
	if (!HasAuthority())
	{
		PrimaryActorTick.bCanEverTick = false;
	}
}

void ASquadCoordinator::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!HasAuthority())
	{
		return;
	}

	TickActiveCarries(DeltaSeconds);

	if (!bAutoEvaluate)
	{
		return;
	}

	EvaluateAccumulator += DeltaSeconds;
	if (EvaluateAccumulator >= EvaluateInterval)
	{
		EvaluateAccumulator = 0.0f;
		EvaluateSynergies();
	}
}

int32 ASquadCoordinator::RegisterEnemy(AEnemyPawn* Enemy, int32 PreferredSquadId)
{
	if (!Enemy)
	{
		return INDEX_NONE;
	}

	RegisteredEnemies.AddUnique(Enemy);
	const int32 Id = PreferredSquadId != INDEX_NONE ? PreferredSquadId : NextSquadId++;
	Enemy->SetSquadId(Id);
	return Id;
}

void ASquadCoordinator::UnregisterEnemy(AEnemyPawn* Enemy)
{
	if (!Enemy)
	{
		return;
	}
	ClearSynergy(Enemy);
	RegisteredEnemies.RemoveAll([Enemy](const TWeakObjectPtr<AEnemyPawn>& Ptr) { return Ptr.Get() == Enemy || !Ptr.IsValid(); });
	ActiveCarriers.Remove(Enemy->GetSquadId());
}

void ASquadCoordinator::ClearSynergy(AEnemyPawn* Enemy)
{
	if (!Enemy)
	{
		return;
	}

	if (AEnemyAIController* AIC = Cast<AEnemyAIController>(Enemy->GetController()))
	{
		AIC->AssignSynergy(EEnemySynergyAction::None, nullptr, Enemy->GetSquadId());
	}

	if (Enemy->MovementComponent)
	{
		Enemy->MovementComponent->DropPassenger();
	}
}

bool ASquadCoordinator::TryAssignCarrySynergy(AEnemyPawn* Carrier, AEnemyPawn* Payload)
{
	if (!Carrier || !Payload || Carrier == Payload)
	{
		return false;
	}

	const EEnemySquadRole CarrierRole = Carrier->GetSquadRole();
	const EEnemySquadRole PayloadRole = Payload->GetSquadRole();
	if (CarrierRole != EEnemySquadRole::Carrier || PayloadRole != EEnemySquadRole::Payload)
	{
		return false;
	}

	const int32 Squad = Carrier->GetSquadId() != INDEX_NONE ? Carrier->GetSquadId() : RegisterEnemy(Carrier);
	Payload->SetSquadId(Squad);

	if (AEnemyAIController* CarrierAI = Cast<AEnemyAIController>(Carrier->GetController()))
	{
		CarrierAI->AssignSynergy(EEnemySynergyAction::CarryAlly, Payload, Squad);
	}
	if (AEnemyAIController* PayloadAI = Cast<AEnemyAIController>(Payload->GetController()))
	{
		PayloadAI->AssignSynergy(EEnemySynergyAction::BeCarried, Carrier, Squad);
	}

	if (Carrier->MovementComponent)
	{
		Carrier->MovementComponent->AttachPassenger(Payload);
	}

	ActiveCarriers.Add(Squad, Carrier);
	return true;
}

void ASquadCoordinator::EvaluateSynergies()
{
	if (!bEnableCarrySynergy)
	{
		return;
	}

	// Prune invalid
	RegisteredEnemies.RemoveAll([](const TWeakObjectPtr<AEnemyPawn>& Ptr) { return !Ptr.IsValid(); });

	TArray<AEnemyPawn*> Carriers;
	TArray<AEnemyPawn*> Payloads;
	for (const TWeakObjectPtr<AEnemyPawn>& Ptr : RegisteredEnemies)
	{
		AEnemyPawn* E = Ptr.Get();
		if (!E || (E->HealthComponent && E->HealthComponent->IsDead()))
		{
			continue;
		}
		if (E->GetSquadRole() == EEnemySquadRole::Carrier)
		{
			Carriers.Add(E);
		}
		else if (E->GetSquadRole() == EEnemySquadRole::Payload)
		{
			Payloads.Add(E);
		}
	}

	for (AEnemyPawn* Carrier : Carriers)
	{
		if (ActiveCarriers.Contains(Carrier->GetSquadId()))
		{
			continue;
		}

		AEnemyPawn* BestPayload = nullptr;
		float BestDist = PairSearchRadius;
		for (AEnemyPawn* Payload : Payloads)
		{
			// Skip payloads already carried
			bool bAlready = false;
			for (const auto& Pair : ActiveCarriers)
			{
				if (AEnemyPawn* C = Pair.Value.Get())
				{
					if (C->MovementComponent && C->MovementComponent->GetMovementMode()
						&& C->MovementComponent->GetMovementMode()->GetPassenger() == Payload)
					{
						bAlready = true;
						break;
					}
				}
			}
			if (bAlready)
			{
				continue;
			}

			const float Dist = FVector::Dist(Carrier->GetActorLocation(), Payload->GetActorLocation());
			if (Dist < BestDist)
			{
				BestDist = Dist;
				BestPayload = Payload;
			}
		}

		if (BestPayload)
		{
			TryAssignCarrySynergy(Carrier, BestPayload);
		}
	}
}

void ASquadCoordinator::TickActiveCarries(float DeltaSeconds)
{
	TArray<int32> ToClear;
	for (const auto& Pair : ActiveCarriers)
	{
		AEnemyPawn* Carrier = Pair.Value.Get();
		if (!Carrier)
		{
			ToClear.Add(Pair.Key);
			continue;
		}

		AActor* Passenger = Carrier->MovementComponent && Carrier->MovementComponent->GetMovementMode()
			? Carrier->MovementComponent->GetMovementMode()->GetPassenger()
			: nullptr;
		if (!Passenger)
		{
			ToClear.Add(Pair.Key);
			continue;
		}

		// Drop when near carrier's combat target
		if (AEnemyAIController* AIC = Cast<AEnemyAIController>(Carrier->GetController()))
		{
			if (UBlackboardComponent* BB = AIC->GetBlackboardComponent())
			{
				if (AActor* Target = Cast<AActor>(BB->GetValueAsObject(FEnemyBlackboardKeys::TargetActor)))
				{
					const float Dist = FVector::Dist(Carrier->GetActorLocation(), Target->GetActorLocation());
					if (Dist <= DropDistanceToTarget)
					{
						Carrier->MovementComponent->DropPassenger();
						AIC->AssignSynergy(EEnemySynergyAction::DropPayload, Passenger, Pair.Key);
						if (AEnemyPawn* Payload = Cast<AEnemyPawn>(Passenger))
						{
							if (AEnemyAIController* PayloadAI = Cast<AEnemyAIController>(Payload->GetController()))
							{
								PayloadAI->AssignSynergy(EEnemySynergyAction::None, nullptr, Pair.Key);
							}
						}
						ToClear.Add(Pair.Key);
					}
				}
			}
		}
	}

	for (int32 Key : ToClear)
	{
		ActiveCarriers.Remove(Key);
	}
}
