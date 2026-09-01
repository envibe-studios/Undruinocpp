#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AI/EnemyTypes.h"
#include "EnemyMovementComponent.generated.h"

class AEnemyPawn;
class UEnemyMovementMode;

UCLASS(ClassGroup = (EnemyAI), meta = (BlueprintSpawnableComponent), BlueprintType, Blueprintable)
class UNDUINOCPP_API UEnemyMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEnemyMovementComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Movement")
	FEnemyMovementParams MovementParams;

	/** Concrete mode instance (Fly/Float/Crawl/Burrow). Created from definition or set directly. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, Category = "Enemy|Movement")
	TObjectPtr<UEnemyMovementMode> MovementMode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Movement")
	TSubclassOf<UEnemyMovementMode> MovementModeClass;

	UFUNCTION(BlueprintCallable, Category = "Enemy|Movement")
	void InitializeFromParams(const FEnemyMovementParams& Params, TSubclassOf<UEnemyMovementMode> ModeClass);

	UFUNCTION(BlueprintCallable, Category = "Enemy|Movement")
	bool MoveToLocation(const FVector& WorldLocation, float AcceptanceRadius = -1.0f);

	/** Focus actor for orbit / dive-bomb combat flight (usually the player ship). */
	UFUNCTION(BlueprintCallable, Category = "Enemy|Movement")
	void SetCombatFocus(AActor* FocusActor);

	UFUNCTION(BlueprintCallable, Category = "Enemy|Movement")
	void StopMovement();

	UFUNCTION(BlueprintCallable, Category = "Enemy|Movement")
	void SetFlockOffset(const FVector& Offset);

	UFUNCTION(BlueprintCallable, Category = "Enemy|Movement")
	void ClearFlockOffset();

	UFUNCTION(BlueprintPure, Category = "Enemy|Movement")
	UEnemyMovementMode* GetMovementMode() const { return MovementMode; }

	UFUNCTION(BlueprintPure, Category = "Enemy|Movement")
	EEnemyMovementState GetMovementState() const;

	UFUNCTION(BlueprintCallable, Category = "Enemy|Movement")
	bool AttachPassenger(AActor* Passenger);

	UFUNCTION(BlueprintCallable, Category = "Enemy|Movement")
	bool DropPassenger();

	float GetSpeedMultiplier() const { return SpeedMultiplier; }
	void SetSpeedMultiplier(float Mult) { SpeedMultiplier = FMath::Max(0.0f, Mult); }

protected:
	void EnsureModeInstance();

	UPROPERTY(Transient)
	TObjectPtr<AEnemyPawn> OwnerEnemy = nullptr;

	float SpeedMultiplier = 1.0f;
};
