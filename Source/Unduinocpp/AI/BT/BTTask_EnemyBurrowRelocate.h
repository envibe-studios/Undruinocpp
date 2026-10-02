#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_EnemyBurrowRelocate.generated.h"

UCLASS()
class UNDUINOCPP_API UBTTask_EnemyBurrowRelocate : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_EnemyBurrowRelocate();

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetActorKey;

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};
