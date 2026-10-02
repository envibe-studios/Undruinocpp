#pragma once

#include "CoreMinimal.h"
#include "AI/EnemyMovementMode.h"
#include "FloatingMovementMode.generated.h"

/** Buoyant / hover-like movement — stays near PreferredAltitude with softer Z response. */
UCLASS(EditInlineNew, DefaultToInstanced, BlueprintType, Blueprintable)
class UNDUINOCPP_API UFloatingMovementMode : public UEnemyMovementMode
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Floating", meta = (ClampMin = "0.0"))
	float HoverDamping = 4.0f;

	virtual void TickMovement(float DeltaTime) override;
};
