#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_EnemyMoveTo.generated.h"

UCLASS()
class UNDUINOCPP_API UBTTask_EnemyMoveTo : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_EnemyMoveTo();

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector MoveGoalKey;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetActorKey;

	UPROPERTY(EditAnywhere, Category = "Move", meta = (ClampMin = "0.0"))
	float AcceptanceRadius = -1.0f;

	UPROPERTY(EditAnywhere, Category = "Move")
	bool bTrackMovingTarget = true;

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};
