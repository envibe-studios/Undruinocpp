#pragma once

#include "CoreMinimal.h"
#include "AI/EnemyPawn.h"
#include "EnemyTurretPawn.generated.h"

class UStationaryTurretComponent;
class USphereComponent;

/**
 * Stationary weapon emplacement enemy.
 * Uses Enemy AI perception/aggro/health + UStationaryTurretComponent for 2-axis aim/fire.
 * Prioritizes ship thrusters (and Engine-tagged parts) when aiming.
 */
UCLASS(Blueprintable)
class UNDUINOCPP_API AEnemyTurretPawn : public AEnemyPawn
{
	GENERATED_BODY()

public:
	AEnemyTurretPawn();

	virtual void BeginPlay() override;
	virtual void ApplyDefinition(UEnemyDefinition* Definition) override;

	/**
	 * Optional hit volume for player weapons. Prefer assigning an existing SphereComponent
	 * on the Blueprint (e.g. DamageCollider). If null and bCreateDamageSphere, one is created.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Turret|Collision", meta = (AllowedClasses = "/Script/Engine.SphereComponent", UseComponentPicker))
	FComponentReference DamageSphereComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Turret|Collision")
	FName DamageSphereTagOrName = FName(TEXT("DamageCollider"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Turret|Collision")
	bool bCreateDamageSphere = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Turret|Collision", meta = (ClampMin = "0.0", EditCondition = "bCreateDamageSphere"))
	float DamageSphereRadius = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Turret|Collision")
	FName DamageSphereCollisionProfile = FName(TEXT("BlockAllDynamic"));

	UFUNCTION(BlueprintPure, Category = "Enemy|Turret")
	UStationaryTurretComponent* GetStationaryTurret() const { return CachedStationaryTurret; }

protected:
	virtual void ConfigureNonPhysicalCollision() override;

	UFUNCTION()
	void HandleTurretDied();

	void ResolveDamageSphere();
	void ConfigureTurretForEnemyAI();
	void CacheTurretComponent();

	/** Resolved from the Blueprint-authored StationaryTurret component (avoid UPROPERTY name clash). */
	UPROPERTY(Transient)
	TObjectPtr<UStationaryTurretComponent> CachedStationaryTurret = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<USphereComponent> ResolvedDamageSphere = nullptr;
};
