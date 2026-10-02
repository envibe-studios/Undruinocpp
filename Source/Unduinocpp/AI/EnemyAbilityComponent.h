#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AI/EnemyTypes.h"
#include "EnemyAbilityComponent.generated.h"

class UEnemyAbility;
class UEnemyAbilityLoadout;
class AEnemyPawn;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEnemyAbilityActivated, FName, AbilityId, AActor*, Target);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemyAbilityFailed, FName, AbilityId);

USTRUCT()
struct FEnemyAbilityRuntime
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UEnemyAbility> Definition = nullptr;

	float CooldownRemaining = 0.0f;
	int32 ChargesRemaining = 0;
};

UCLASS(ClassGroup = (EnemyAI), meta = (BlueprintSpawnableComponent), BlueprintType, Blueprintable)
class UNDUINOCPP_API UEnemyAbilityComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEnemyAbilityComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Abilities")
	TObjectPtr<UEnemyAbilityLoadout> Loadout;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Abilities")
	TArray<TObjectPtr<UEnemyAbility>> ExtraAbilities;

	UPROPERTY(BlueprintAssignable, Category = "Enemy|Abilities")
	FOnEnemyAbilityActivated OnAbilityActivated;

	UPROPERTY(BlueprintAssignable, Category = "Enemy|Abilities")
	FOnEnemyAbilityFailed OnAbilityFailed;

	UFUNCTION(BlueprintCallable, Category = "Enemy|Abilities")
	void InitializeFromLoadout(UEnemyAbilityLoadout* InLoadout);

	UFUNCTION(BlueprintCallable, Category = "Enemy|Abilities")
	bool CanActivateAbility(FName AbilityId, AActor* OptionalTarget = nullptr) const;

	UFUNCTION(BlueprintCallable, Category = "Enemy|Abilities")
	bool ActivateAbility(FName AbilityId, AActor* OptionalTarget = nullptr);

	UFUNCTION(BlueprintPure, Category = "Enemy|Abilities")
	float GetCooldownRemaining(FName AbilityId) const;

	UFUNCTION(BlueprintPure, Category = "Enemy|Abilities")
	TArray<FName> GetAbilityIds() const;

	void SetCooldownScale(float Scale) { CooldownScale = FMath::Max(0.05f, Scale); }
	float GetCooldownScale() const { return CooldownScale; }

protected:
	AActor* ResolveTarget(const UEnemyAbility* Ability, AActor* OptionalTarget) const;
	bool ApplyEffect(const UEnemyAbility* Ability, AActor* Target);
	FEnemyAbilityRuntime* FindRuntime(FName AbilityId);
	const FEnemyAbilityRuntime* FindRuntime(FName AbilityId) const;
	void RebuildRuntime();

	UPROPERTY(Transient)
	TArray<FEnemyAbilityRuntime> RuntimeAbilities;

	UPROPERTY(Transient)
	TObjectPtr<AEnemyPawn> OwnerEnemy = nullptr;

	float CooldownScale = 1.0f;
};
