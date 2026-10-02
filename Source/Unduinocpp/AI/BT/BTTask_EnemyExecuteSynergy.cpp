#include "AI/BT/BTTask_EnemyExecuteSynergy.h"
#include "AI/EnemyAIController.h"
#include "AI/EnemyPawn.h"
#include "AI/EnemyMovementComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AI/EnemyTypes.h"

UBTTask_EnemyExecuteSynergy::UBTTask_EnemyExecuteSynergy()
{
	NodeName = TEXT("Enemy Execute Synergy");
}

EBTNodeResult::Type UBTTask_EnemyExecuteSynergy::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AEnemyAIController* AIC = Cast<AEnemyAIController>(OwnerComp.GetAIOwner());
	AEnemyPawn* Enemy = AIC ? Cast<AEnemyPawn>(AIC->GetPawn()) : nullptr;
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!Enemy || !BB || !Enemy->MovementComponent)
	{
		return EBTNodeResult::Failed;
	}

	const EEnemySynergyAction Action = static_cast<EEnemySynergyAction>(BB->GetValueAsEnum(FEnemyBlackboardKeys::SynergyAction));
	AActor* Partner = Cast<AActor>(BB->GetValueAsObject(FEnemyBlackboardKeys::SynergyPartner));

	switch (Action)
	{
	case EEnemySynergyAction::CarryAlly:
		if (Partner && Enemy->MovementComponent->AttachPassenger(Partner))
		{
			return EBTNodeResult::Succeeded;
		}
		return EBTNodeResult::Failed;
	case EEnemySynergyAction::DropPayload:
		Enemy->MovementComponent->DropPassenger();
		BB->SetValueAsEnum(FEnemyBlackboardKeys::SynergyAction, static_cast<uint8>(EEnemySynergyAction::None));
		return EBTNodeResult::Succeeded;
	case EEnemySynergyAction::BeCarried:
		// Passive — partner attaches us.
		return EBTNodeResult::Succeeded;
	default:
		return EBTNodeResult::Failed;
	}
}
