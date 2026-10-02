#pragma once

#include "CoreMinimal.h"
#include "AI/EnemyMovementMode.h"
#include "CrawlingMovementMode.generated.h"

/** Ground / navmesh-oriented crawling. Uses simple ground projection when nav is unavailable. */
UCLASS(EditInlineNew, DefaultToInstanced, BlueprintType, Blueprintable)
class UNDUINOCPP_API UCrawlingMovementMode : public UEnemyMovementMode
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crawling")
	bool bProjectToNavMesh = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crawling", meta = (ClampMin = "0.0"))
	float GroundTraceHeight = 200.0f;

	virtual void TickMovement(float DeltaTime) override;
	virtual bool MoveToLocation(const FVector& WorldLocation, float AcceptanceRadiusOverride = -1.0f) override;

private:
	FVector ProjectGoalToGround(const FVector& Desired) const;
};
