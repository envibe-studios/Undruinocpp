#pragma once

#include "CoreMinimal.h"
#include "AI/EnemyMovementMode.h"
#include "BurrowingMovementMode.generated.h"

/** Leaves collision/visibility, relocates, then emerges. BT-friendly enter/exit. */
UCLASS(EditInlineNew, DefaultToInstanced, BlueprintType, Blueprintable)
class UNDUINOCPP_API UBurrowingMovementMode : public UEnemyMovementMode
{
	GENERATED_BODY()

public:
	virtual void TickMovement(float DeltaTime) override;
	virtual bool CanBurrow() const override { return true; }
	virtual bool BeginBurrow() override;
	virtual bool EndBurrowAt(const FVector& EmergeLocation) override;

	/** Call from BT to pick a random relocate point and burrow there. */
	UFUNCTION(BlueprintCallable, Category = "Enemy|Burrow")
	bool StartBurrowRelocate(const FVector& PreferredEmergeLocation);

	UFUNCTION(BlueprintPure, Category = "Enemy|Burrow")
	bool IsBurrowInProgress() const { return bBurrowInProgress; }

private:
	bool bBurrowInProgress = false;
	bool bHiddenUnderground = false;
	float BurrowElapsed = 0.0f;
	FVector EmergeLocation = FVector::ZeroVector;
	TEnumAsByte<ECollisionEnabled::Type> CachedCollision = ECollisionEnabled::QueryAndPhysics;
};
