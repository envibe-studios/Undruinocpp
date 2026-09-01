#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "AI/EnemyTypes.h"
#include "EnemyPawn.generated.h"

class UCapsuleComponent;
class USkeletalMeshComponent;
class UEnemyHealthComponent;
class UEnemyMovementComponent;
class UEnemyAbilityComponent;
class UAggroComponent;
class UEnemyDefinition;

UCLASS(Blueprintable)
class UNDUINOCPP_API AEnemyPawn : public APawn
{
	GENERATED_BODY()

public:
	AEnemyPawn();

	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy")
	TObjectPtr<UCapsuleComponent> CapsuleComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy")
	TObjectPtr<USkeletalMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy")
	TObjectPtr<UEnemyHealthComponent> HealthComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy")
	TObjectPtr<UEnemyMovementComponent> MovementComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy")
	TObjectPtr<UEnemyAbilityComponent> AbilityComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy")
	TObjectPtr<UAggroComponent> AggroComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	TObjectPtr<UEnemyDefinition> EnemyDefinition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Debug")
	bool bDrawDebug = false;

	/**
	 * If true (default), enemy collision is QueryOnly so dive/chase contacts
	 * damage the ship without physically launching or flipping it.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Collision")
	bool bDisablePhysicalShipCollision = true;

	UFUNCTION(BlueprintCallable, Category = "Enemy")
	virtual void ApplyDefinition(UEnemyDefinition* Definition);

	UFUNCTION(BlueprintPure, Category = "Enemy")
	FVector GetHomeLocation() const { return HomeLocation; }

	UFUNCTION(BlueprintPure, Category = "Enemy")
	UEnemyDefinition* GetEnemyDefinition() const { return EnemyDefinition; }

	UFUNCTION(BlueprintCallable, Category = "Enemy")
	AActor* FindNearestAlly(float MaxRange) const;

	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void SetSquadId(int32 InSquadId);

	UFUNCTION(BlueprintPure, Category = "Enemy")
	int32 GetSquadId() const { return SquadId; }

	UFUNCTION(BlueprintPure, Category = "Enemy")
	EEnemySquadRole GetSquadRole() const { return SquadRole; }

protected:
	UFUNCTION()
	void HandleDied();

	void ApplyDefaultTags();
	virtual void ConfigureNonPhysicalCollision();

	FVector HomeLocation = FVector::ZeroVector;

	UPROPERTY(Replicated)
	int32 SquadId = INDEX_NONE;

	UPROPERTY(Replicated)
	EEnemySquadRole SquadRole = EEnemySquadRole::None;
};
