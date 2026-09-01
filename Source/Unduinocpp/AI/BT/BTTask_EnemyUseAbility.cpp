#include "AI/BT/BTTask_EnemyUseAbility.h"
#include "AI/EnemyAIController.h"
#include "AI/EnemyPawn.h"
#include "AI/EnemyAbilityComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AI/EnemyTypes.h"

UBTTask_EnemyUseAbility::UBTTask_EnemyUseAbility()
{
	NodeName = TEXT("Enemy Use Ability");
	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_EnemyUseAbility, TargetActorKey), AActor::StaticClass());
}

EBTNodeResult::Type UBTTask_EnemyUseAbility::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AEnemyAIController* AIC = Cast<AEnemyAIController>(OwnerComp.GetAIOwner());
	AEnemyPawn* Enemy = AIC ? Cast<AEnemyPawn>(AIC->GetPawn()) : nullptr;
	if (!Enemy || !Enemy->AbilityComponent || AbilityId.IsNone())
	{
		return EBTNodeResult::Failed;
	}

	AActor* Target = nullptr;
	if (UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent())
	{
		const FName Key = TargetActorKey.SelectedKeyName.IsNone() ? FEnemyBlackboardKeys::TargetActor : TargetActorKey.SelectedKeyName;
		Target = Cast<AActor>(BB->GetValueAsObject(Key));
	}

	return Enemy->AbilityComponent->ActivateAbility(AbilityId, Target) ? EBTNodeResult::Succeeded : EBTNodeResult::Failed;
}
