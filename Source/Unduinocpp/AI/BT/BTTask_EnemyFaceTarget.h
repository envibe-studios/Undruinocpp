#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_EnemyFaceTarget.generated.h"

UCLASS()
class UNDUINOCPP_API UBTTask_EnemyFaceTarget : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_EnemyFaceTarget();

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetActorKey;

	UPROPERTY(EditAnywhere, Category = "Face", meta = (ClampMin = "0.0"))
	float AngleToleranceDeg = 10.0f;

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};
