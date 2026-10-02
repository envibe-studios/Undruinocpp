#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_EnemyUseAbility.generated.h"

UCLASS()
class UNDUINOCPP_API UBTTask_EnemyUseAbility : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_EnemyUseAbility();

	UPROPERTY(EditAnywhere, Category = "Ability")
	FName AbilityId;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetActorKey;

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
