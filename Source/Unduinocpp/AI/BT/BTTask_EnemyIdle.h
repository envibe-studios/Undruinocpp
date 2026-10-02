#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_EnemyIdle.generated.h"

UCLASS()
class UNDUINOCPP_API UBTTask_EnemyIdle : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_EnemyIdle();

	UPROPERTY(EditAnywhere, Category = "Idle", meta = (ClampMin = "0.0"))
	float IdleDuration = 1.0f;

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	virtual uint16 GetInstanceMemorySize() const override { return sizeof(float); }
};
