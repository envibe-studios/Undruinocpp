// Mission Game State - Implementation

#include "MissionGameState.h"
#include "Net/UnrealNetwork.h"

AMissionGameState::AMissionGameState()
{
	// Replication is default for GameState
}

void AMissionGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMissionGameState, GlobalThreatLevel);
	DOREPLIFETIME(AMissionGameState, FactionThreatLevels);
	DOREPLIFETIME(AMissionGameState, MissionStates);
	DOREPLIFETIME(AMissionGameState, ObjectiveProgress);
}

void AMissionGameState::OnRep_MissionStates()
{
	OnReplicatedMissionDataChanged.Broadcast();
}

void AMissionGameState::OnRep_ObjectiveProgress()
{
	OnReplicatedMissionDataChanged.Broadcast();
}

EMissionState AMissionGameState::GetMissionState(FName MissionID) const
{
	for (const FReplicatedMissionState& Entry : MissionStates)
	{
		if (Entry.MissionID == MissionID)
		{
			return static_cast<EMissionState>(Entry.State);
		}
	}
	return EMissionState::Hidden;
}

int32 AMissionGameState::GetFactionThreatLevel(FName FactionID) const
{
	if (FactionID.IsNone()) return 0;
	for (const FReplicatedFactionThreat& Entry : FactionThreatLevels)
	{
		if (Entry.FactionID == FactionID)
		{
			return Entry.ThreatLevel;
		}
	}
	return 0;
}

void AMissionGameState::GetObjectiveProgress(FName MissionID, int32 ObjectiveIndex, bool& bFound, FMissionObjectiveProgress& OutProgress) const
{
	bFound = false;
	for (const FMissionObjectiveProgress& P : ObjectiveProgress)
	{
		if (P.MissionID == MissionID && P.ObjectiveIndex == ObjectiveIndex)
		{
			OutProgress = P;
			bFound = true;
			return;
		}
	}
}
