#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_EnemyExecuteSynergy.generated.h"

/** Reacts to SquadCoordinator-assigned SynergyAction (Carry / Drop). */
UCLASS()
class UNDUINOCPP_API UBTTask_EnemyExecuteSynergy : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_EnemyExecuteSynergy();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
