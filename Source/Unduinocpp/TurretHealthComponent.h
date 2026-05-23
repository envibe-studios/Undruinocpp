// TurretHealthComponent - health, optional collision sphere, explosion on death

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "TurretHealthComponent.generated.h"

class USphereComponent;
class UDamageType;
class UParticleSystem;
class USoundBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTurretDamaged, float, NewHitpoints, float, DamageAmount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTurretHealed, float, NewHitpoints, float, HealAmount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTurretDestroyed);

UCLASS(ClassGroup=(Weapon), meta=(BlueprintSpawnableComponent), BlueprintType, Blueprintable)
class UNDUINOCPP_API UTurretHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTurretHealthComponent();

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ============================================================================
	// HEALTH
	// ============================================================================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Turret|Health", meta=(ClampMin="0.0"))
	float MaxHitpoints = 200.0f;

	UPROPERTY(ReplicatedUsing=OnRep_CurrentHitpoints, EditAnywhere, BlueprintReadWrite, Category="Turret|Health", meta=(ClampMin="0.0"))
	float CurrentHitpoints = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Turret|Health")
	bool bCanBeDamaged = true;

	/** Optional damage multipliers by damage type (1 = normal, 0.5 = resistant, 2 = vulnerable). Only used if non-empty. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Turret|Health", meta=(DisplayName="Damage Type Multipliers"))
	TMap<TSubclassOf<UDamageType>, float> DamageTypeMultipliers;

	UFUNCTION(BlueprintCallable, Category="Turret|Health")
	float ApplyDamage(float DamageAmount, TSubclassOf<UDamageType> DamageTypeClass);

	UFUNCTION(BlueprintCallable, Category="Turret|Health")
	float Heal(float HealAmount);

	UFUNCTION(BlueprintCallable, Category="Turret|Health")
	void SetHitpoints(float NewHitpoints);

	UFUNCTION(BlueprintPure, Category="Turret|Health")
	bool IsDestroyed() const { return CurrentHitpoints <= 0.0f; }

	// ============================================================================
	// COLLISION (optional)
	// ============================================================================

	/** Optional: assign an existing SphereComponent in the owning BP so you can move/scale it in-editor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Turret|Collision", meta=(AllowedClasses="/Script/Engine.SphereComponent", UseComponentPicker))
	FComponentReference DamageSphereComponent;

	/** Fallback: find a sphere component by Component Tag (or by component variable name). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Turret|Collision")
	FName DamageSphereTagOrName = NAME_None;

	/** If true, creates/registers a USphereComponent at runtime for receiving hits/overlaps/traces. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Turret|Collision")
	bool bCreateDamageSphere = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Turret|Collision", meta=(ClampMin="0.0", EditCondition="bCreateDamageSphere"))
	float DamageSphereRadius = 60.0f;

	/** Collision profile to apply to the created sphere. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Turret|Collision", meta=(EditCondition="bCreateDamageSphere"))
	FName DamageSphereCollisionProfile = FName("BlockAllDynamic");

	UFUNCTION(BlueprintPure, Category="Turret|Collision")
	USphereComponent* GetDamageSphere() const { return DamageSphere; }

	// ============================================================================
	// EXPLOSION
	// ============================================================================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Turret|Explosion")
	TObjectPtr<UParticleSystem> ExplosionFX = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Turret|Explosion")
	TObjectPtr<USoundBase> ExplosionSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Turret|Explosion")
	bool bDestroyOwnerOnDeath = true;

	// ============================================================================
	// EVENTS
	// ============================================================================

	UPROPERTY(BlueprintAssignable, Category="Turret|Events")
	FOnTurretDamaged OnTurretDamaged;

	UPROPERTY(BlueprintAssignable, Category="Turret|Events")
	FOnTurretHealed OnTurretHealed;

	UPROPERTY(BlueprintAssignable, Category="Turret|Events")
	FOnTurretDestroyed OnTurretDestroyed;

protected:
	UFUNCTION()
	void OnRep_CurrentHitpoints();

	UFUNCTION()
	void HandleOwnerTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
		AController* InstigatedBy, AActor* DamageCauser);

	void Explode();

private:
	UPROPERTY(Transient)
	TObjectPtr<USphereComponent> DamageSphere = nullptr;

	float PreviousHitpoints = 0.0f;
	bool bHasExploded = false;
};

