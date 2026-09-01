#include "AI/EnemyAIController.h"
#include "AI/EnemyPawn.h"
#include "AI/EnemyDefinition.h"
#include "AI/EnemyAbilityComponent.h"
#include "AI/AggroComponent.h"
#include "AI/EnemyMovementComponent.h"
#include "AI/EnemyMovementMode.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISense_Sight.h"
#include "GameFramework/PlayerController.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "BrainComponent.h"

AEnemyAIController::AEnemyAIController()
{
	PrimaryActorTick.bCanEverTick = true;
	bWantsPlayerState = false;

	PerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComp"));
	SetPerceptionComponent(*PerceptionComp);

	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = 5000.0f;
	SightConfig->LoseSightRadius = 6000.0f;
	SightConfig->PeripheralVisionAngleDegrees = 90.0f;
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = false;
	SightConfig->SetMaxAge(5.0f);

	PerceptionComp->ConfigureSense(*SightConfig);
	PerceptionComp->SetDominantSense(UAISense_Sight::StaticClass());
}

void AEnemyAIController::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		PrimaryActorTick.bCanEverTick = false;
		return;
	}

	if (PerceptionComp)
	{
		PerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &AEnemyAIController::HandleTargetPerceptionUpdated);
	}
}

void AEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	CachedEnemy = Cast<AEnemyPawn>(InPawn);
	if (CachedEnemy && HasAuthority())
	{
		InitializeFromPawn(CachedEnemy);
		StartBehavior();
	}
}

void AEnemyAIController::OnUnPossess()
{
	StopBehavior();
	CachedEnemy = nullptr;
	Super::OnUnPossess();
}

void AEnemyAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!HasAuthority())
	{
		return;
	}

	UpdateLOD(DeltaSeconds);
	UpdateAutoPursuit();

	if (bDrawDebug || (CachedEnemy && CachedEnemy->bDrawDebug))
	{
		DrawDebugOverlay();
	}
}

void AEnemyAIController::UpdateAutoPursuit()
{
	if (!bAutoPursueTarget || !CachedEnemy || !CachedEnemy->MovementComponent)
	{
		return;
	}

	const UBlackboardComponent* BB = GetBlackboardComponent();
	if (!BB)
	{
		return;
	}

	AActor* Target = Cast<AActor>(BB->GetValueAsObject(FEnemyBlackboardKeys::TargetActor));
	if (!IsValid(Target))
	{
		if (CachedEnemy->MovementComponent->GetMovementMode()
			&& CachedEnemy->MovementComponent->GetMovementMode()->GetCombatFocus())
		{
			CachedEnemy->MovementComponent->SetCombatFocus(nullptr);
			CachedEnemy->MovementComponent->StopMovement();
		}
		return;
	}

	CachedEnemy->MovementComponent->SetCombatFocus(Target);
	CachedEnemy->MovementComponent->MoveToLocation(Target->GetActorLocation());
}

void AEnemyAIController::InitializeFromPawn(AEnemyPawn* Enemy)
{
	CachedEnemy = Enemy;
	if (!Enemy)
	{
		return;
	}

	if (Enemy->GetEnemyDefinition())
	{
		InitializeFromDefinition(Enemy->GetEnemyDefinition());
	}

	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		BB->SetValueAsVector(FEnemyBlackboardKeys::HomeLocation, Enemy->GetHomeLocation());
		BB->SetValueAsFloat(FEnemyBlackboardKeys::AggressionMultiplier, 1.0f);
		BB->SetValueAsFloat(FEnemyBlackboardKeys::AbilityCooldownScale, 1.0f);
		BB->SetValueAsEnum(FEnemyBlackboardKeys::AssignedRole, static_cast<uint8>(Enemy->GetSquadRole()));
		SetCombatState(EEnemyCombatState::Idle);
	}
}

void AEnemyAIController::InitializeFromDefinition(UEnemyDefinition* Definition)
{
	CachedDefinition = Definition;
	if (!Definition)
	{
		return;
	}

	TargetPolicy = Definition->TargetPolicy;
	bAutoPursueTarget = Definition->bAutoPursueTarget;
	ConfigureSight(Definition->PerceptionParams);

	if (Definition->BehaviorTree)
	{
		DefaultBehaviorTree = Definition->BehaviorTree;
	}
	if (Definition->BlackboardAsset)
	{
		DefaultBlackboard = Definition->BlackboardAsset;
	}

	if (CachedEnemy && CachedEnemy->AggroComponent)
	{
		CachedEnemy->AggroComponent->AggroParams = Definition->AggroParams;
	}
}

void AEnemyAIController::ConfigureSight(const FEnemyPerceptionParams& Params)
{
	if (!SightConfig || !PerceptionComp)
	{
		return;
	}

	SightConfig->SightRadius = Params.SightRadius;
	SightConfig->LoseSightRadius = Params.LoseSightRadius;
	SightConfig->PeripheralVisionAngleDegrees = Params.PeripheralVisionAngleDeg;
	SightConfig->DetectionByAffiliation.bDetectEnemies = Params.bDetectEnemies;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = Params.bDetectNeutrals;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = Params.bDetectFriendlies;
	PerceptionComp->ConfigureSense(*SightConfig);
	PerceptionComp->RequestStimuliListenerUpdate();
}

void AEnemyAIController::EnsureBlackboardKeys()
{
}

void AEnemyAIController::StartBehavior()
{
	if (!HasAuthority() || bBehaviorRunning)
	{
		return;
	}

	UBehaviorTree* Tree = DefaultBehaviorTree;
	if (!Tree)
	{
		return;
	}

	UBlackboardData* BBAsset = DefaultBlackboard.Get();
	if (!BBAsset)
	{
		BBAsset = Tree->GetBlackboardAsset();
	}
	if (BBAsset)
	{
		UBlackboardComponent* BlackboardComp = Blackboard.Get();
		UseBlackboard(BBAsset, BlackboardComp);
	}

	if (CachedEnemy)
	{
		InitializeFromPawn(CachedEnemy);
	}

	RunBehaviorTree(Tree);
	bBehaviorRunning = true;
}

void AEnemyAIController::StopBehavior()
{
	if (BrainComponent)
	{
		BrainComponent->StopLogic(TEXT("StopBehavior"));
	}
	bBehaviorRunning = false;
}

void AEnemyAIController::OnPossessedPawnDied()
{
	SetCombatState(EEnemyCombatState::Dead);
	StopBehavior();
}

void AEnemyAIController::SetCombatState(EEnemyCombatState NewState)
{
	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		BB->SetValueAsEnum(FEnemyBlackboardKeys::CombatState, static_cast<uint8>(NewState));
	}
}

void AEnemyAIController::SetAggressionMultiplier(float Multiplier)
{
	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		BB->SetValueAsFloat(FEnemyBlackboardKeys::AggressionMultiplier, Multiplier);
	}
}

void AEnemyAIController::SetAbilityCooldownScale(float Scale)
{
	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		BB->SetValueAsFloat(FEnemyBlackboardKeys::AbilityCooldownScale, Scale);
	}
	if (CachedEnemy && CachedEnemy->AbilityComponent)
	{
		CachedEnemy->AbilityComponent->SetCooldownScale(Scale);
	}
}

void AEnemyAIController::AssignSynergy(EEnemySynergyAction Action, AActor* Partner, int32 InSquadId)
{
	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		BB->SetValueAsInt(FEnemyBlackboardKeys::SynergyAction, static_cast<int32>(Action));
		BB->SetValueAsObject(FEnemyBlackboardKeys::SynergyPartner, Partner);
		BB->SetValueAsInt(FEnemyBlackboardKeys::SquadId, InSquadId);
	}
	if (CachedEnemy)
	{
		CachedEnemy->SetSquadId(InSquadId);
	}
}

void AEnemyAIController::HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Actor || !CachedEnemy || !CachedEnemy->AggroComponent)
	{
		return;
	}

	if (Stimulus.WasSuccessfullySensed())
	{
		const bool bIsEnemy = Actor->ActorHasTag(FName(TEXT("Enemy")));
		if (!bIsEnemy)
		{
			CachedEnemy->AggroComponent->NotifySightOn(Actor);
		}
	}

	UpdateTargetFromPolicy();
}

TArray<AActor*> AEnemyAIController::GetPerceivedHostileActors() const
{
	TArray<AActor*> Out;
	if (!PerceptionComp)
	{
		return Out;
	}

	TArray<AActor*> Perceived;
	PerceptionComp->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), Perceived);

	for (AActor* Actor : Perceived)
	{
		if (!Actor || Actor == GetPawn())
		{
			continue;
		}

		if (Actor->ActorHasTag(FName(TEXT("Enemy"))))
		{
			continue;
		}

		if (APawn* AsPawn = Cast<APawn>(Actor))
		{
			Out.Add(AsPawn);
			continue;
		}

		if (Actor->ActorHasTag(FName(TEXT("Targetable"))))
		{
			Out.Add(Actor);
		}
	}

	return Out;
}

void AEnemyAIController::UpdateTargetFromPolicy()
{
	if (!CachedEnemy)
	{
		return;
	}

	UBlackboardComponent* BB = GetBlackboardComponent();
	AActor* Current = BB ? Cast<AActor>(BB->GetValueAsObject(FEnemyBlackboardKeys::TargetActor)) : nullptr;
	const TArray<AActor*> Perceived = GetPerceivedHostileActors();

	AActor* Selected = nullptr;
	if (CachedEnemy->AggroComponent)
	{
		Selected = CachedEnemy->AggroComponent->SelectTarget(TargetPolicy, Current, Perceived);
	}
	else if (Perceived.Num() > 0)
	{
		Selected = Perceived[0];
	}

	if (!BB)
	{
		return;
	}

	BB->SetValueAsObject(FEnemyBlackboardKeys::TargetActor, Selected);
	if (Selected)
	{
		BB->SetValueAsVector(FEnemyBlackboardKeys::LastKnownLocation, Selected->GetActorLocation());
		BB->SetValueAsEnum(FEnemyBlackboardKeys::CombatState, static_cast<uint8>(EEnemyCombatState::Combat));
	}
	else if (Current)
	{
		BB->SetValueAsEnum(FEnemyBlackboardKeys::CombatState, static_cast<uint8>(EEnemyCombatState::Alert));
	}
}

void AEnemyAIController::UpdateLOD(float DeltaSeconds)
{
	APawn* Controlled = GetPawn();
	if (!Controlled)
	{
		return;
	}

	float DistToPlayer = 0.0f;
	bool bHasPlayer = false;
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			if (APawn* PlayerPawn = PC->GetPawn())
			{
				DistToPlayer = FVector::Dist(Controlled->GetActorLocation(), PlayerPawn->GetActorLocation());
				bHasPlayer = true;
			}
		}
	}

	const float DesiredInterval = (bHasPlayer && DistToPlayer > FarLODDistance) ? FarTickInterval : NearTickInterval;
	if (DesiredInterval <= 0.0f)
	{
		if (BrainComponent)
		{
			BrainComponent->ResumeLogic(TEXT("LODNear"));
		}
		return;
	}

	LODAccumulator += DeltaSeconds;
	if (LODAccumulator < DesiredInterval)
	{
		if (BrainComponent)
		{
			BrainComponent->PauseLogic(TEXT("LODFar"));
		}
	}
	else
	{
		LODAccumulator = 0.0f;
		if (BrainComponent)
		{
			BrainComponent->ResumeLogic(TEXT("LODPulse"));
		}
	}
}

void AEnemyAIController::DrawDebugOverlay() const
{
	APawn* Controlled = GetPawn();
	UWorld* World = GetWorld();
	if (!Controlled || !World)
	{
		return;
	}

	const FVector Loc = Controlled->GetActorLocation();
	FString ModeName = TEXT("None");
	if (CachedEnemy && CachedEnemy->MovementComponent && CachedEnemy->MovementComponent->GetMovementMode())
	{
		ModeName = CachedEnemy->MovementComponent->GetMovementMode()->GetClass()->GetName();
	}

	DrawDebugString(World, Loc + FVector(0, 0, 100), FString::Printf(TEXT("Mode: %s"), *ModeName), nullptr, FColor::Cyan, 0.0f, true);

	if (const UBlackboardComponent* BB = GetBlackboardComponent())
	{
		if (const AActor* Target = Cast<AActor>(BB->GetValueAsObject(FEnemyBlackboardKeys::TargetActor)))
		{
			DrawDebugLine(World, Loc, Target->GetActorLocation(), FColor::Red, false, -1.0f, 0, 2.0f);
			DrawDebugString(World, Loc + FVector(0, 0, 80), FString::Printf(TEXT("Target: %s"), *Target->GetName()), nullptr, FColor::Orange, 0.0f, true);
		}
	}
}
