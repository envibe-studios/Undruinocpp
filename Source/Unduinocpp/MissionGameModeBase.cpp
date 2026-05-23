// Mission Game Mode Base - Implementation

#include "MissionGameModeBase.h"
#include "MissionGameState.h"
#include "MissionPlayerState.h"

AMissionGameModeBase::AMissionGameModeBase()
{
	GameStateClass = AMissionGameState::StaticClass();
	PlayerStateClass = AMissionPlayerState::StaticClass();
}

void AMissionGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	UGameInstance* GI = GetGameInstance();
	if (!GI) return;

	UMissionManagerSubsystem* Subsystem = GI->GetSubsystem<UMissionManagerSubsystem>();
	if (Subsystem && MissionRegistry)
	{
		Subsystem->RegisterMissionsFromRegistry(MissionRegistry);
	}
}
