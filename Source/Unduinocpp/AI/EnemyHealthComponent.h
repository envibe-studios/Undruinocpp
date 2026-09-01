// Enemy health — shared damage contract for AI pawns (mirrors turret health patterns).

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EnemyHealthComponent.generated.h"

class UDamageType;
class UParticleSystem;
class USoundBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEnemyDamaged, float, NewHitpoints, float, DamageAmount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEnemyHealed, float, NewHitpoints, float, HealAmount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEnemyDied);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemyShieldChanged, float, NewShield);

UCLASS(ClassGroup = (EnemyAI), meta = (BlueprintSpawnableComponent), BlueprintType, Blueprintable)
class UNDUINOCPP_API UEnemyHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEnemyHealthComponent();

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Health", meta = (ClampMin = "0.0"))
	float MaxHitpoints = 100.0f;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentHitpoints, EditAnywhere, BlueprintReadWrite, Category = "Enemy|Health", meta = (ClampMin = "0.0"))
	float CurrentHitpoints = 100.0f;

	UPROPERTY(ReplicatedUsing = OnRep_Shield, EditAnywhere, BlueprintReadWrite, Category = "Enemy|Health", meta = (ClampMin = "0.0"))
	float ShieldHitpoints = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Health", meta = (ClampMin = "0.0"))
	float MaxShieldHitpoints = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Health")
	bool bCanBeDamaged = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Health")
	TMap<TSubclassOf<UDamageType>, float> DamageTypeMultipliers;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Death")
	TObjectPtr<UParticleSystem> DeathFX = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Death")
	TObjectPtr<USoundBase> DeathSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Death")
	bool bDestroyOwnerOnDeath = true;

	UPROPERTY(BlueprintAssignable, Category = "Enemy|Events")
	FOnEnemyDamaged OnEnemyDamaged;

	UPROPERTY(BlueprintAssignable, Category = "Enemy|Events")
	FOnEnemyHealed OnEnemyHealed;

	UPROPERTY(BlueprintAssignable, Category = "Enemy|Events")
	FOnEnemyDied OnEnemyDied;

	UPROPERTY(BlueprintAssignable, Category = "Enemy|Events")
	FOnEnemyShieldChanged OnEnemyShieldChanged;

	UFUNCTION(BlueprintCallable, Category = "Enemy|Health")
	float ApplyDamage(float DamageAmount, AController* InstigatedBy, AActor* DamageCauser, TSubclassOf<UDamageType> DamageTypeClass = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Enemy|Health")
	float Heal(float HealAmount);

	UFUNCTION(BlueprintCallable, Category = "Enemy|Health")
	void AddShield(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Enemy|Health")
	void SetHitpoints(float NewHitpoints);

	UFUNCTION(BlueprintPure, Category = "Enemy|Health")
	bool IsDead() const { return CurrentHitpoints <= 0.0f; }

	UFUNCTION(BlueprintPure, Category = "Enemy|Health")
	float GetHealthPercent() const { return MaxHitpoints > 0.0f ? CurrentHitpoints / MaxHitpoints : 0.0f; }

protected:
	UFUNCTION()
	void OnRep_CurrentHitpoints();

	UFUNCTION()
	void OnRep_Shield();

	UFUNCTION()
	void HandleOwnerTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
		AController* InstigatedBy, AActor* DamageCauser);

	void HandleDeath();

private:
	float PreviousHitpoints = 0.0f;
	float PreviousShield = 0.0f;
	bool bHasDied = false;
};
