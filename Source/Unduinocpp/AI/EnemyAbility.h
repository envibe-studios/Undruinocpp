#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AI/EnemyTypes.h"
#include "EnemyAbility.generated.h"

class AActor;

/**
 * Lightweight ability definition (not GAS).
 * BT activates by AbilityId / tag via UEnemyAbilityComponent.
 */
UCLASS(BlueprintType)
class UNDUINOCPP_API UEnemyAbility : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability|Identity")
	FName AbilityId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability|Identity")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability|Timing", meta = (ClampMin = "0.0"))
	float CooldownSeconds = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability|Timing", meta = (ClampMin = "0.0"))
	float CastTimeSeconds = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability|Range", meta = (ClampMin = "0.0"))
	float MinRange = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability|Range", meta = (ClampMin = "0.0"))
	float MaxRange = 3000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	EEnemyAbilityEffectType EffectType = EEnemyAbilityEffectType::SpawnProjectile;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	EEnemyAbilityTargetRule TargetRule = EEnemyAbilityTargetRule::CurrentTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability|Effect", meta = (ClampMin = "0.0"))
	float Magnitude = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability|Effect", meta = (ClampMin = "0.0"))
	float DurationSeconds = 0.0f;

	/** Optional projectile for SpawnProjectile abilities. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability|Projectile")
	TSubclassOf<AActor> ProjectileClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability|Projectile", meta = (ClampMin = "0.0"))
	float ProjectileSpeed = 4000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	bool bInterruptible = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability", meta = (ClampMin = "0"))
	int32 MaxCharges = 0;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("EnemyAbility"), AbilityId.IsNone() ? GetFName() : AbilityId);
	}
};

UCLASS(BlueprintType)
class UNDUINOCPP_API UEnemyAbilityLoadout : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loadout")
	TArray<TObjectPtr<UEnemyAbility>> Abilities;
};
