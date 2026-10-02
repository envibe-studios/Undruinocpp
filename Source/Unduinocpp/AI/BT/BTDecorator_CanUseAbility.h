#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_CanUseAbility.generated.h"

UCLASS()
class UNDUINOCPP_API UBTDecorator_CanUseAbility : public UBTDecorator
{
	GENERATED_BODY()

public:
	UBTDecorator_CanUseAbility();

	UPROPERTY(EditAnywhere, Category = "Ability")
	FName AbilityId;

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
};
