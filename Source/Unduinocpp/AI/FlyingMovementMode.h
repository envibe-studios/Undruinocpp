#pragma once

#include "CoreMinimal.h"
#include "AI/EnemyMovementMode.h"
#include "FlyingMovementMode.generated.h"

class UHoverThrusterComponent;

/**
 * Arcade combat flight: soar/sway on approach, wide orbit, dive-bomb ship parts (thrusters).
 * MoveGoal / combat focus actor are the ship — pathing is not a straight line.
 */
UCLASS(EditInlineNew, DefaultToInstanced, BlueprintType, Blueprintable)
class UNDUINOCPP_API UFlyingMovementMode : public UEnemyMovementMode
{
	GENERATED_BODY()

public:
	virtual void Initialize(AEnemyPawn* InOwner, UEnemyMovementComponent* InMovement) override;
	virtual void TickMovement(float DeltaTime) override;
	virtual bool MoveToLocation(const FVector& WorldLocation, float AcceptanceRadiusOverride = -1.0f) override;
	virtual void SetCombatFocus(AActor* FocusActor) override;
	virtual void ClearMoveGoal() override;
	virtual void StopMovement() override;

	// --- Tunables ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Orbit", meta = (ClampMin = "100.0"))
	float OrbitRadius = 2200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Orbit", meta = (ClampMin = "100.0"))
	float OrbitEnterDistance = 2800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Orbit", meta = (ClampMin = "0.0"))
	float OrbitHeightVariance = 350.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Sway", meta = (ClampMin = "0.0"))
	float SwayAmplitudeXY = 450.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Sway", meta = (ClampMin = "0.0"))
	float SwayAmplitudeZ = 220.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Sway", meta = (ClampMin = "0.05"))
	float SwayFrequencyA = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Sway", meta = (ClampMin = "0.05"))
	float SwayFrequencyB = 0.37f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Sway", meta = (ClampMin = "0.05"))
	float SwayFrequencyC = 0.71f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Dive", meta = (ClampMin = "0.5"))
	float DiveCooldownMin = 3.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Dive", meta = (ClampMin = "0.5"))
	float DiveCooldownMax = 7.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Dive", meta = (ClampMin = "1.0"))
	float DiveSpeedMultiplier = 1.85f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Dive", meta = (ClampMin = "0.0"))
	float DiveDamage = 28.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Dive", meta = (ClampMin = "50.0"))
	float DiveHitRadius = 280.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Dive", meta = (ClampMin = "100.0"))
	float PullUpDistance = 900.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Dive", meta = (ClampMin = "0.0"))
	float PullUpClimbHeight = 700.0f;

	/** Soft COM velocity nudge on a successful dive hit (cm/s). 0 = damage only. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Dive", meta = (ClampMin = "0.0"))
	float DiveNudgeSpeed = 140.0f;

	/** How much vertical component is allowed in the nudge (0 = flat shove only). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Dive", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DiveNudgeVerticalScale = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Debug")
	bool bDrawFlightDebug = false;

	UFUNCTION(BlueprintPure, Category = "Flight")
	EEnemyFlyingCombatPhase GetCombatPhase() const { return CombatPhase; }

protected:
	void ResetDiveTimer();
	FVector ComputeSwayOffset(float TimeSeconds) const;
	FVector GetFocusLocation() const;
	bool HasValidFocus() const;
	void PickDiveTarget();
	void TryApplyDiveDamage();
	void ApplySoftDiveNudge(AActor* Ship) const;
	void FaceVelocity(float DeltaTime, const FVector& Velocity);
	FVector SteerToward(const FVector& DesiredWorldPoint, float Speed) const;
	void TickApproach(float DeltaTime);
	void TickOrbit(float DeltaTime);
	void TickDive(float DeltaTime);
	void TickPullUp(float DeltaTime);
	void EnterPhase(EEnemyFlyingCombatPhase NewPhase);

	EEnemyFlyingCombatPhase CombatPhase = EEnemyFlyingCombatPhase::Approach;

	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> FocusActor;

	UPROPERTY(Transient)
	TWeakObjectPtr<UHoverThrusterComponent> DiveThruster;

	FVector DiveAimPoint = FVector::ZeroVector;
	FVector PullUpPoint = FVector::ZeroVector;
	float OrbitAngle = 0.0f;
	float OrbitDirection = 1.0f;
	float DiveCooldownRemaining = 0.0f;
	float ClosestDiveDistance = TNumericLimits<float>::Max();
	bool bDiveDamageApplied = false;
	float NoisePhaseA = 0.0f;
	float NoisePhaseB = 0.0f;
	float NoisePhaseC = 0.0f;
};
