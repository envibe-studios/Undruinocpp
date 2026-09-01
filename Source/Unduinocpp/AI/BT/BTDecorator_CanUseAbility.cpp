#include "AI/BT/BTDecorator_CanUseAbility.h"
#include "AI/EnemyAIController.h"
#include "AI/EnemyPawn.h"
#include "AI/EnemyAbilityComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AI/EnemyTypes.h"

UBTDecorator_CanUseAbility::UBTDecorator_CanUseAbility()
{
	NodeName = TEXT("Can Use Ability");
}

bool UBTDecorator_CanUseAbility::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	AEnemyAIController* AIC = Cast<AEnemyAIController>(OwnerComp.GetAIOwner());
	AEnemyPawn* Enemy = AIC ? Cast<AEnemyPawn>(AIC->GetPawn()) : nullptr;
	if (!Enemy || !Enemy->AbilityComponent || AbilityId.IsNone())
	{
		return false;
	}

	AActor* Target = nullptr;
	if (UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent())
	{
		Target = Cast<AActor>(BB->GetValueAsObject(FEnemyBlackboardKeys::TargetActor));
	}
	return Enemy->AbilityComponent->CanActivateAbility(AbilityId, Target);
}
