#pragma once

#include "CoreMinimal.h"
#include "AI/EnemyMovementMode.h"
#include "StationaryMovementMode.generated.h"

/** Fixed emplacement — no translation. Combat focus may optionally yaw the pawn root. */
UCLASS(EditInlineNew, DefaultToInstanced, BlueprintType, Blueprintable)
class UNDUINOCPP_API UStationaryMovementMode : public UEnemyMovementMode
{
	GENERATED_BODY()

public:
	/** If true, yaw the pawn toward CombatFocus each tick (turret meshes usually handle aim instead). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stationary")
	bool bFaceCombatFocus = false;

	virtual void TickMovement(float DeltaTime) override;
	virtual bool MoveToLocation(const FVector& WorldLocation, float AcceptanceRadiusOverride = -1.0f) override;
};
