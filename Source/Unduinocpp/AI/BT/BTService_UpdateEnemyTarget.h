#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_UpdateEnemyTarget.generated.h"

UCLASS()
class UNDUINOCPP_API UBTService_UpdateEnemyTarget : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_UpdateEnemyTarget();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};
