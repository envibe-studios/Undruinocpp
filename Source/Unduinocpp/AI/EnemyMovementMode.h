// Abstract movement mode — Fly / Float / Crawl / Burrow plug into UEnemyMovementComponent.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "AI/EnemyTypes.h"
#include "EnemyMovementMode.generated.h"

class AEnemyPawn;
class UEnemyMovementComponent;

UCLASS(Abstract, EditInlineNew, DefaultToInstanced, BlueprintType, Blueprintable)
class UNDUINOCPP_API UEnemyMovementMode : public UObject
{
	GENERATED_BODY()

public:
	virtual void Initialize(AEnemyPawn* InOwner, UEnemyMovementComponent* InMovement);
	virtual void Shutdown();

	virtual void TickMovement(float DeltaTime);
	virtual void StopMovement();

	/** Request move toward a world location. Returns true when within acceptance radius. */
	virtual bool MoveToLocation(const FVector& WorldLocation, float AcceptanceRadiusOverride = -1.0f);

	/** Optional focus actor for combat flight (ship). Modes may orbit / dive relative to this. */
	virtual void SetCombatFocus(AActor* FocusActor) { CombatFocusActor = FocusActor; }

	UFUNCTION(BlueprintPure, Category = "Enemy|Movement")
	AActor* GetCombatFocus() const { return CombatFocusActor.Get(); }

	/** Optional flocking offset applied on top of current goal. */
	virtual void SetFlockOffset(const FVector& WorldOffset);

	virtual void ClearMoveGoal();

	UFUNCTION(BlueprintPure, Category = "Enemy|Movement")
	bool HasMoveGoal() const { return bHasMoveGoal; }

	UFUNCTION(BlueprintPure, Category = "Enemy|Movement")
	FVector GetMoveGoal() const { return MoveGoal; }

	UFUNCTION(BlueprintPure, Category = "Enemy|Movement")
	EEnemyMovementState GetMovementState() const { return MovementState; }

	/** Synergy hooks — override in modes that support carrying. */
	virtual bool AttachPassenger(AActor* Passenger);
	virtual bool DropPassenger();
	virtual AActor* GetPassenger() const { return PassengerActor.Get(); }

	virtual bool CanBurrow() const { return false; }
	virtual bool BeginBurrow();
	virtual bool EndBurrowAt(const FVector& EmergeLocation);

	void ApplyParams(const FEnemyMovementParams& InParams);

protected:
	void FaceDirection(float DeltaTime, const FVector& WorldDirection);
	void MoveOwnerToward(float DeltaTime, const FVector& DesiredVelocity, bool bAutoFace = true);

	UPROPERTY(Transient)
	TObjectPtr<AEnemyPawn> OwnerPawn = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UEnemyMovementComponent> MovementComp = nullptr;

	FEnemyMovementParams Params;
	FVector MoveGoal = FVector::ZeroVector;
	FVector FlockOffset = FVector::ZeroVector;
	/** Local velocity cache — APawn::GetVelocity() is usually 0 without physics/CMC. */
	FVector CurrentVelocity = FVector::ZeroVector;
	bool bHasMoveGoal = false;
	bool bHasFlockOffset = false;
	EEnemyMovementState MovementState = EEnemyMovementState::Normal;

	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> PassengerActor;

	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> CombatFocusActor;
};
