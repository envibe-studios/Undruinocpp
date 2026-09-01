#include "AI/EnemyAIDirectorSubsystem.h"
#include "AI/EnemyAIController.h"
#include "MissionManagerSubsystem.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

bool UEnemyAIDirectorSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (const UWorld* World = Cast<UWorld>(Outer))
	{
		return World->IsGameWorld();
	}
	return false;
}

void UEnemyAIDirectorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	BindMissionEvents();
}

void UEnemyAIDirectorSubsystem::Deinitialize()
{
	UnbindMissionEvents();
	Super::Deinitialize();
}

void UEnemyAIDirectorSubsystem::BindMissionEvents()
{
	if (bBound)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (UGameInstance* GI = World->GetGameInstance())
	{
		if (UMissionManagerSubsystem* Missions = GI->GetSubsystem<UMissionManagerSubsystem>())
		{
			Missions->OnThreatChanged.AddDynamic(this, &UEnemyAIDirectorSubsystem::HandleThreatChanged);
			Missions->OnThreatThresholdCrossed.AddDynamic(this, &UEnemyAIDirectorSubsystem::HandleThreatThreshold);
			bBound = true;
		}
	}
}

void UEnemyAIDirectorSubsystem::UnbindMissionEvents()
{
	if (!bBound)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GI = World->GetGameInstance())
		{
			if (UMissionManagerSubsystem* Missions = GI->GetSubsystem<UMissionManagerSubsystem>())
			{
				Missions->OnThreatChanged.RemoveDynamic(this, &UEnemyAIDirectorSubsystem::HandleThreatChanged);
				Missions->OnThreatThresholdCrossed.RemoveDynamic(this, &UEnemyAIDirectorSubsystem::HandleThreatThreshold);
			}
		}
	}
	bBound = false;
}

void UEnemyAIDirectorSubsystem::HandleThreatChanged(int32 NewThreatLevel, int32 PreviousThreatLevel)
{
	ApplyThreatToAllEnemies(NewThreatLevel);
}

void UEnemyAIDirectorSubsystem::HandleThreatThreshold(int32 NewThreatLevel, int32 Threshold, bool bAboveThreshold)
{
	if (bAboveThreshold)
	{
		ApplyThreatToAllEnemies(NewThreatLevel);
	}
}

void UEnemyAIDirectorSubsystem::ApplyThreatToAllEnemies(int32 ThreatLevel)
{
	UWorld* World = GetWorld();
	if (!World || !World->GetAuthGameMode())
	{
		return;
	}

	const float Aggression = BaseAggression + static_cast<float>(ThreatLevel) * AggressionPerThreat;
	const float CooldownScale = FMath::Clamp(1.0f - static_cast<float>(ThreatLevel) * CooldownScalePerThreat, MinCooldownScale, 1.0f);

	for (FConstControllerIterator It = World->GetControllerIterator(); It; ++It)
	{
		if (AEnemyAIController* AIC = Cast<AEnemyAIController>(It->Get()))
		{
			AIC->SetAggressionMultiplier(Aggression);
			AIC->SetAbilityCooldownScale(CooldownScale);
		}
	}
}
