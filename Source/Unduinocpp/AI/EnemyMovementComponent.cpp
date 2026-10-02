#include "AI/EnemyMovementComponent.h"
#include "AI/EnemyMovementMode.h"
#include "AI/EnemyPawn.h"
#include "AI/FlyingMovementMode.h"

UEnemyMovementComponent::UEnemyMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UEnemyMovementComponent::BeginPlay()
{
	Super::BeginPlay();
	OwnerEnemy = Cast<AEnemyPawn>(GetOwner());
	EnsureModeInstance();
	if (MovementMode)
	{
		MovementMode->Initialize(OwnerEnemy, this);
	}
}

void UEnemyMovementComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (MovementMode)
	{
		MovementMode->Shutdown();
	}
	Super::EndPlay(EndPlayReason);
}

void UEnemyMovementComponent::EnsureModeInstance()
{
	if (MovementMode)
	{
		return;
	}

	const TSubclassOf<UEnemyMovementMode> ClassToSpawn = MovementModeClass
		? MovementModeClass
		: TSubclassOf<UEnemyMovementMode>(UFlyingMovementMode::StaticClass());

	MovementMode = NewObject<UEnemyMovementMode>(this, ClassToSpawn);
}

void UEnemyMovementComponent::InitializeFromParams(const FEnemyMovementParams& Params, TSubclassOf<UEnemyMovementMode> ModeClass)
{
	MovementParams = Params;
	if (ModeClass)
	{
		MovementModeClass = ModeClass;
		if (MovementMode)
		{
			MovementMode->Shutdown();
			MovementMode = nullptr;
		}
	}
	EnsureModeInstance();
	if (MovementMode)
	{
		MovementMode->Initialize(OwnerEnemy ? OwnerEnemy.Get() : Cast<AEnemyPawn>(GetOwner()), this);
	}
}

void UEnemyMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!MovementMode)
	{
		return;
	}

	FEnemyMovementParams Scaled = MovementParams;
	Scaled.MaxSpeed *= SpeedMultiplier;
	MovementMode->ApplyParams(Scaled);
	MovementMode->TickMovement(DeltaTime);
}

bool UEnemyMovementComponent::MoveToLocation(const FVector& WorldLocation, float AcceptanceRadius)
{
	return MovementMode ? MovementMode->MoveToLocation(WorldLocation, AcceptanceRadius) : false;
}

void UEnemyMovementComponent::SetCombatFocus(AActor* FocusActor)
{
	if (MovementMode)
	{
		MovementMode->SetCombatFocus(FocusActor);
	}
}

void UEnemyMovementComponent::StopMovement()
{
	if (MovementMode)
	{
		MovementMode->StopMovement();
	}
}

void UEnemyMovementComponent::SetFlockOffset(const FVector& Offset)
{
	if (MovementMode)
	{
		MovementMode->SetFlockOffset(Offset);
	}
}

void UEnemyMovementComponent::ClearFlockOffset()
{
	if (MovementMode)
	{
		MovementMode->SetFlockOffset(FVector::ZeroVector);
	}
}

EEnemyMovementState UEnemyMovementComponent::GetMovementState() const
{
	return MovementMode ? MovementMode->GetMovementState() : EEnemyMovementState::Normal;
}

bool UEnemyMovementComponent::AttachPassenger(AActor* Passenger)
{
	return MovementMode ? MovementMode->AttachPassenger(Passenger) : false;
}

bool UEnemyMovementComponent::DropPassenger()
{
	return MovementMode ? MovementMode->DropPassenger() : false;
}
