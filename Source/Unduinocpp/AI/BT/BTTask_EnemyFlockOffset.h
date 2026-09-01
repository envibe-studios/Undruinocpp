#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_EnemyFlockOffset.generated.h"

UCLASS()
class UNDUINOCPP_API UBTTask_EnemyFlockOffset : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_EnemyFlockOffset();

	UPROPERTY(EditAnywhere, Category = "Flocking", meta = (ClampMin = "0.0"))
	float AllySearchRadius = 1500.0f;

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
