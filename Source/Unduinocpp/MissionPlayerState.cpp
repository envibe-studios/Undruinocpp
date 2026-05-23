// Mission Player State - Implementation

#include "MissionPlayerState.h"
#include "MissionDataAsset.h"
#include "Net/UnrealNetwork.h"

AMissionPlayerState::AMissionPlayerState()
{
	SetReplicates(true);
}

void AMissionPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AMissionPlayerState, PerPlayerObjectiveProgress);
}

int32 AMissionPlayerState::ServerApplyPerPlayerObjectiveProgress(FName MissionID, int32 ObjectiveIndex, int32 DeltaCount, int32 TargetCount, float TimeLimitSeconds)
{
	if (!HasAuthority() || DeltaCount <= 0 || MissionID.IsNone() || TargetCount <= 0) return -1;

	int32 FoundIndex = -1;
	for (int32 i = 0; i < PerPlayerObjectiveProgress.Num(); ++i)
	{
		if (PerPlayerObjectiveProgress[i].MissionID == MissionID && PerPlayerObjectiveProgress[i].ObjectiveIndex == ObjectiveIndex)
		{
			FoundIndex = i;
			break;
		}
	}
	if (FoundIndex < 0)
	{
		FMissionObjectiveProgress P;
		P.MissionID = MissionID;
		P.ObjectiveIndex = ObjectiveIndex;
		P.CurrentCount = 0;
		P.bCompleted = false;
		P.TimeRemaining = TimeLimitSeconds > 0.0f ? TimeLimitSeconds : 0.0f;
		PerPlayerObjectiveProgress.Add(P);
		FoundIndex = PerPlayerObjectiveProgress.Num() - 1;
	}

	if (PerPlayerObjectiveProgress[FoundIndex].bCompleted) return PerPlayerObjectiveProgress[FoundIndex].CurrentCount;

	PerPlayerObjectiveProgress[FoundIndex].CurrentCount = FMath::Min(PerPlayerObjectiveProgress[FoundIndex].CurrentCount + DeltaCount, TargetCount);
	if (PerPlayerObjectiveProgress[FoundIndex].CurrentCount >= TargetCount)
	{
		PerPlayerObjectiveProgress[FoundIndex].bCompleted = true;
	}
	return PerPlayerObjectiveProgress[FoundIndex].CurrentCount;
}

void AMissionPlayerState::GetPerPlayerObjectiveProgress(FName MissionID, int32 ObjectiveIndex, bool& bFound, FMissionObjectiveProgress& OutProgress) const
{
	bFound = false;
	for (const FMissionObjectiveProgress& P : PerPlayerObjectiveProgress)
	{
		if (P.MissionID == MissionID && P.ObjectiveIndex == ObjectiveIndex)
		{
			OutProgress = P;
			bFound = true;
			return;
		}
	}
}

bool AMissionPlayerState::IsPerPlayerObjectiveComplete(FName MissionID, int32 ObjectiveIndex, int32 TargetCount) const
{
	FMissionObjectiveProgress P;
	bool bFound = false;
	GetPerPlayerObjectiveProgress(MissionID, ObjectiveIndex, bFound, P);
	if (!bFound) return false;
	return P.bCompleted || P.CurrentCount >= TargetCount;
}

bool AMissionPlayerState::AreAllPerPlayerObjectivesCompleteForMission(FName MissionID, const UMissionDataAsset* MissionData) const
{
	if (!MissionData || MissionData->MissionID != MissionID) return false;
	for (int32 i = 0; i < MissionData->Objectives.Num(); ++i)
	{
		if (MissionData->Objectives[i].ObjectiveScope != EObjectiveScope::PerPlayer) continue;
		int32 Target = MissionData->Objectives[i].TargetCount;
		if (!IsPerPlayerObjectiveComplete(MissionID, i, Target)) return false;
	}
	return true;
}
