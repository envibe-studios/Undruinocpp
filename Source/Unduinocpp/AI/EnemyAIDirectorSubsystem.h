#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "EnemyAIDirectorSubsystem.generated.h"

class AEnemyAIController;
class UMissionManagerSubsystem;

/**
 * Director-lite: listens to mission threat and scales enemy aggression / ability cooldowns.
 */
UCLASS()
class UNDUINOCPP_API UEnemyAIDirectorSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Director")
	float BaseAggression = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Director")
	float AggressionPerThreat = 0.02f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Director")
	float MinCooldownScale = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Director")
	float CooldownScalePerThreat = 0.01f;

	UFUNCTION(BlueprintCallable, Category = "AI Director")
	void ApplyThreatToAllEnemies(int32 ThreatLevel);

protected:
	UFUNCTION()
	void HandleThreatChanged(int32 NewThreatLevel, int32 PreviousThreatLevel);

	UFUNCTION()
	void HandleThreatThreshold(int32 NewThreatLevel, int32 Threshold, bool bAboveThreshold);

	void BindMissionEvents();
	void UnbindMissionEvents();

	bool bBound = false;
};
