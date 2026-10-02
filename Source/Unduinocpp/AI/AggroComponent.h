#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AI/EnemyTypes.h"
#include "AggroComponent.generated.h"

class AEnemyAIController;

USTRUCT(BlueprintType)
struct FAggroEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Aggro")
	TWeakObjectPtr<AActor> Target;

	UPROPERTY(BlueprintReadOnly, Category = "Aggro")
	float Aggro = 0.0f;
};

UCLASS(ClassGroup = (EnemyAI), meta = (BlueprintSpawnableComponent), BlueprintType, Blueprintable)
class UNDUINOCPP_API UAggroComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAggroComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Aggro")
	FEnemyAggroParams AggroParams;

	UFUNCTION(BlueprintCallable, Category = "Enemy|Aggro")
	void AddAggro(AActor* Target, float Amount);

	UFUNCTION(BlueprintCallable, Category = "Enemy|Aggro")
	void ClearAggro(AActor* Target);

	UFUNCTION(BlueprintCallable, Category = "Enemy|Aggro")
	void ClearAllAggro();

	UFUNCTION(BlueprintPure, Category = "Enemy|Aggro")
	AActor* GetHighestAggroTarget() const;

	UFUNCTION(BlueprintPure, Category = "Enemy|Aggro")
	float GetAggro(AActor* Target) const;

	UFUNCTION(BlueprintCallable, Category = "Enemy|Aggro")
	void NotifyDamagedBy(AActor* DamageCauser, float DamageAmount);

	UFUNCTION(BlueprintCallable, Category = "Enemy|Aggro")
	void NotifySightOn(AActor* SeenActor);

	/** Writes best target to Blackboard when controller is available. */
	UFUNCTION(BlueprintCallable, Category = "Enemy|Aggro")
	AActor* SelectTarget(EEnemyTargetPolicy Policy, AActor* CurrentTarget, const TArray<AActor*>& PerceivedActors) const;

protected:
	UFUNCTION()
	void HandleOwnerTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
		AController* InstigatedBy, AActor* DamageCauser);

	UPROPERTY(Transient)
	TArray<FAggroEntry> AggroTable;
};
