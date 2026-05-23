// Mission Manager Subsystem - Implementation

#include "MissionManagerSubsystem.h"
#include "MissionPlayerState.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Pawn.h"

UMissionManagerSubsystem::UMissionManagerSubsystem()
{
}

void UMissionManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

bool UMissionManagerSubsystem::IsTickable() const
{
	const UGameInstance* GI = GetGameInstance();
	const UWorld* World = GI ? GI->GetWorld() : nullptr;
	return World != nullptr && !IsTemplate();
}

TStatId UMissionManagerSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UMissionManagerSubsystem, STATGROUP_Tickables);
}

UWorld* UMissionManagerSubsystem::GetTickableGameObjectWorld() const
{
	const UGameInstance* GI = GetGameInstance();
	return GI ? GI->GetWorld() : nullptr;
}

void UMissionManagerSubsystem::RegisterMissionsFromRegistry(UMissionRegistryAsset* Registry)
{
	if (!Registry) return;

	UWorld* World = GetGameInstance()->GetWorld();
	if (!World) return;

	AMissionGameState* GS = GetOrFindMissionGameState();
	if (!GS) return;

	MissionDataMap.Empty();
	ThreatThresholds = Registry->ThreatThresholds;
	SessionTimeSeconds = 0.0f;

	for (UMissionDataAsset* MissionData : Registry->Missions)
	{
		if (!MissionData || MissionData->MissionID.IsNone()) continue;
		MissionDataMap.Add(MissionData->MissionID, MissionData);
	}

	// Clients need mission definitions for UI, but only the server is allowed to mutate replicated state.
	if (!HasAuthority())
	{
		return;
	}

	// Set initial mission states: Main = Active, others = Hidden
	for (const auto& Pair : MissionDataMap)
	{
		FName MissionID = Pair.Key;
		UMissionDataAsset* Data = Pair.Value;
		EMissionState InitialState = (Data->MissionType == EMissionType::Main && MissionID == Registry->MainMissionID)
			? EMissionState::Active
			: EMissionState::Hidden;
		EnsureMissionStateInReplicatedArrays(MissionID, InitialState);

		if (InitialState == EMissionState::Active)
		{
			// Initialize session-scoped objective progress only (PerPlayer lives on MissionPlayerState)
			for (int32 i = 0; i < Data->Objectives.Num(); ++i)
			{
				if (Data->Objectives[i].ObjectiveScope == EObjectiveScope::PerPlayer) continue;
				FMissionObjectiveProgress P;
				P.MissionID = MissionID;
				P.ObjectiveIndex = i;
				P.CurrentCount = 0;
				P.bCompleted = false;
				int32 TargetCount = Data->Objectives[i].TargetCount;
				if (Data->bRandomizeTargetCount && Data->MinTargetCount <= Data->MaxTargetCount)
				{
					TargetCount = FMath::RandRange(Data->MinTargetCount, Data->MaxTargetCount);
				}
				P.TimeRemaining = Data->Objectives[i].TimeLimitSeconds > 0.0f ? Data->Objectives[i].TimeLimitSeconds : 0.0f;
				GS->ObjectiveProgress.Add(P);
			}
			OnMissionActivated.Broadcast(MissionID, Data);
		}
	}

	// Ensure global threat is in GameState (already 0 by default)
	LastBroadcastThreatLevel = GS->GlobalThreatLevel;
}

void UMissionManagerSubsystem::RegisterMission(UMissionDataAsset* MissionData)
{
	if (!MissionData || MissionData->MissionID.IsNone()) return;

	MissionDataMap.Add(MissionData->MissionID, MissionData);
	if (HasAuthority())
	{
		EnsureMissionStateInReplicatedArrays(MissionData->MissionID, EMissionState::Hidden);
	}
}

EMissionState UMissionManagerSubsystem::GetMissionState(FName MissionID) const
{
	AMissionGameState* GS = GetOrFindMissionGameState();
	return GS ? GS->GetMissionState(MissionID) : EMissionState::Hidden;
}

int32 UMissionManagerSubsystem::GetGlobalThreatLevel() const
{
	AMissionGameState* GS = GetOrFindMissionGameState();
	return GS ? GS->GlobalThreatLevel : 0;
}

int32 UMissionManagerSubsystem::GetFactionThreatLevel(FName FactionID) const
{
	AMissionGameState* GS = GetOrFindMissionGameState();
	return GS ? GS->GetFactionThreatLevel(FactionID) : 0;
}

AMissionGameState* UMissionManagerSubsystem::GetMissionGameState() const
{
	return GetOrFindMissionGameState();
}

UMissionDataAsset* UMissionManagerSubsystem::GetMissionData(FName MissionID) const
{
	const TObjectPtr<UMissionDataAsset>* Found = MissionDataMap.Find(MissionID);
	return Found ? *Found : nullptr;
}

void UMissionManagerSubsystem::ReportObjectiveProgress(FName MissionID, int32 ObjectiveIndex, int32 DeltaCount, EMissionRole ReporterRole)
{
	if (!HasAuthority() || DeltaCount == 0) return;

	AMissionGameState* GS = GetOrFindMissionGameState();
	UMissionDataAsset* Data = GetMissionData(MissionID);
	if (!GS || !Data) return;

	if (GS->GetMissionState(MissionID) != EMissionState::Active) return;
	if (ObjectiveIndex < 0 || ObjectiveIndex >= Data->Objectives.Num()) return;

	const FMissionObjectiveDef& ObjDef = Data->Objectives[ObjectiveIndex];
	if (ObjDef.ObjectiveScope == EObjectiveScope::PerPlayer) return; // use ReportObjectiveProgressForPlayer
	if (ObjDef.RequiredRole != EMissionRole::Any && ObjDef.RequiredRole != ReporterRole)
	{
		UE_LOG(LogTemp, Warning, TEXT("MissionManager: ReportObjectiveProgress %s [%d] rejected - objective RequiredRole=%d, ReporterRole=%d (must match or set objective to Any)."),
			*MissionID.ToString(), ObjectiveIndex, static_cast<int32>(ObjDef.RequiredRole), static_cast<int32>(ReporterRole));
		return;
	}

	int32 FoundIndex = -1;
	for (int32 i = 0; i < GS->ObjectiveProgress.Num(); ++i)
	{
		if (GS->ObjectiveProgress[i].MissionID == MissionID && GS->ObjectiveProgress[i].ObjectiveIndex == ObjectiveIndex)
		{
			FoundIndex = i;
			break;
		}
	}
	if (FoundIndex < 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("MissionManager: ReportObjectiveProgress %s [%d] no progress row (mission may not be Active or objective not Session)."),
			*MissionID.ToString(), ObjectiveIndex);
		return;
	}

	int32 TargetCount = ObjDef.TargetCount;
	GS->ObjectiveProgress[FoundIndex].CurrentCount = FMath::Min(GS->ObjectiveProgress[FoundIndex].CurrentCount + DeltaCount, TargetCount);
	OnObjectiveProgress.Broadcast(MissionID, ObjectiveIndex, GS->ObjectiveProgress[FoundIndex].CurrentCount);

		if (GS->ObjectiveProgress[FoundIndex].CurrentCount >= TargetCount)
		{
			GS->ObjectiveProgress[FoundIndex].bCompleted = true;

			// Check if all SESSION objectives for this mission are complete
			bool bAllSessionComplete = true;
			for (const FMissionObjectiveProgress& P : GS->ObjectiveProgress)
			{
				if (P.MissionID != MissionID) continue;
				if (!P.bCompleted) { bAllSessionComplete = false; break; }
			}
			// Only complete the mission when ALL objectives (session + per-player) are satisfied
			bool bShouldComplete = bAllSessionComplete;
			if (bShouldComplete)
			{
				// If mission has PerPlayer objectives, require them to be satisfied before completing
				bool bHasPerPlayer = false;
				for (const FMissionObjectiveDef& O : Data->Objectives)
				{
					if (O.ObjectiveScope == EObjectiveScope::PerPlayer) { bHasPerPlayer = true; break; }
				}
				if (bHasPerPlayer)
				{
					if (Data->bCompleteWhenAllPlayersFinishPerPlayerObjectives)
						bShouldComplete = AreAllPlayersPerPlayerObjectivesComplete(MissionID, Data);
					else
					{
						bShouldComplete = false; // don't complete from session progress alone when mission has per-player objectives
						UE_LOG(LogTemp, Log, TEXT("MissionManager: %s has PerPlayer objectives and bCompleteWhenAllPlayersFinishPerPlayerObjectives is false; need per-player completion."), *MissionID.ToString());
					}
				}
			}
			UE_LOG(LogTemp, Log, TEXT("MissionManager: Session objective complete %s [%d], bAllSessionComplete=%d bShouldComplete=%d"),
				*MissionID.ToString(), ObjectiveIndex, bAllSessionComplete, bShouldComplete);
			if (bShouldComplete)
			{
				CompleteMission(MissionID, true);
			}
		}
}

void UMissionManagerSubsystem::ReportObjectiveProgressForPlayer(APlayerState* PlayerState, FName MissionID, int32 ObjectiveIndex, int32 DeltaCount, EMissionRole ReporterRole)
{
	if (!HasAuthority() || DeltaCount == 0 || !PlayerState) return;

	AMissionGameState* GS = GetOrFindMissionGameState();
	UMissionDataAsset* Data = GetMissionData(MissionID);
	AMissionPlayerState* MPS = Cast<AMissionPlayerState>(PlayerState);
	if (!GS || !Data || !MPS)
	{
		if (!MPS)
		{
			UE_LOG(LogTemp, Warning, TEXT("MissionManager: ReportObjectiveProgressForPlayer %s [%d] - PlayerState is not MissionPlayerState. Set GameMode Default Player State Class to MissionPlayerState (or BP child)."),
				*MissionID.ToString(), ObjectiveIndex);
		}
		return;
	}
	if (GS->GetMissionState(MissionID) != EMissionState::Active) return;
	if (ObjectiveIndex < 0 || ObjectiveIndex >= Data->Objectives.Num()) return;

	const FMissionObjectiveDef& ObjDef = Data->Objectives[ObjectiveIndex];
	if (ObjDef.ObjectiveScope != EObjectiveScope::PerPlayer)
	{
		UE_LOG(LogTemp, Warning, TEXT("MissionManager: ReportObjectiveProgressForPlayer called for %s [%d] but objective is Session (use ReportObjectiveProgress instead). No progress applied."),
			*MissionID.ToString(), ObjectiveIndex);
		return;
	}
	if (ObjDef.RequiredRole != EMissionRole::Any && ObjDef.RequiredRole != ReporterRole)
	{
		UE_LOG(LogTemp, Warning, TEXT("MissionManager: ReportObjectiveProgressForPlayer %s [%d] rejected - objective RequiredRole=%d, ReporterRole=%d (must match or set objective to Any)."),
			*MissionID.ToString(), ObjectiveIndex, static_cast<int32>(ObjDef.RequiredRole), static_cast<int32>(ReporterRole));
		return;
	}

	int32 TargetCount = ObjDef.TargetCount;
	if (Data->bRandomizeTargetCount && Data->MinTargetCount <= Data->MaxTargetCount)
		TargetCount = FMath::Clamp(TargetCount, Data->MinTargetCount, Data->MaxTargetCount);
	// Per-player target is fixed per mission asset; if you need per-player random, use separate mission assets or extend later.

	int32 NewCount = MPS->ServerApplyPerPlayerObjectiveProgress(MissionID, ObjectiveIndex, DeltaCount, TargetCount, ObjDef.TimeLimitSeconds);
	if (NewCount < 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("MissionManager: ReportObjectiveProgressForPlayer %s [%d] - ServerApplyPerPlayerObjectiveProgress returned -1 (check server authority, DeltaCount>0, TargetCount>0)."),
			*MissionID.ToString(), ObjectiveIndex);
		return;
	}

	OnPerPlayerObjectiveProgress.Broadcast(PlayerState, MissionID, ObjectiveIndex, NewCount);
	// Also broadcast session-style progress so UI bound only to OnObjectiveProgress still updates
	OnObjectiveProgress.Broadcast(MissionID, ObjectiveIndex, NewCount);

	// Mission completes when session objectives are all done and either no per-player gate or all players finished per-player objectives
	// Also complete when mission has ONLY per-player objectives and all players have finished them (no checkbox required)
	bool bAllSession = AreAllSessionObjectivesComplete(MissionID, Data, GS);
	bool bAllPerPlayer = AreAllPlayersPerPlayerObjectivesComplete(MissionID, Data);
	bool bOnlyPerPlayerObjectives = (Data->Objectives.Num() > 0);
	for (const FMissionObjectiveDef& O : Data->Objectives)
	{
		if (O.ObjectiveScope == EObjectiveScope::Session) { bOnlyPerPlayerObjectives = false; break; }
	}
	bool bShouldComplete = (Data->bCompleteWhenAllPlayersFinishPerPlayerObjectives && bAllSession && bAllPerPlayer)
		|| (bOnlyPerPlayerObjectives && bAllPerPlayer);
	UE_LOG(LogTemp, Log, TEXT("MissionManager: Per-player objective complete %s [%d], bAllSession=%d bAllPerPlayer=%d bOnlyPerPlayer=%d -> Complete=%d"),
		*MissionID.ToString(), ObjectiveIndex, bAllSession, bAllPerPlayer, bOnlyPerPlayerObjectives, bShouldComplete);
	if (bShouldComplete)
	{
		CompleteMission(MissionID, true);
	}
}

void UMissionManagerSubsystem::ReportObjectiveProgressForPawn(APawn* Pawn, FName MissionID, int32 ObjectiveIndex, int32 DeltaCount, EMissionRole ReporterRole)
{
	if (!Pawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("MissionManager: ReportObjectiveProgressForPawn - Pawn is null. Pass the player pawn (e.g. damage instigator or overlapping pawn)."));
		return;
	}
	APlayerState* PS = Pawn->GetPlayerState();
	if (!PS)
	{
		UE_LOG(LogTemp, Warning, TEXT("MissionManager: ReportObjectiveProgressForPawn %s [%d] - Pawn has no PlayerState (unpossessed or not a player pawn). Use the pawn that is possessed by the player controller."),
			*MissionID.ToString(), ObjectiveIndex);
		return;
	}
	ReportObjectiveProgressForPlayer(PS, MissionID, ObjectiveIndex, DeltaCount, ReporterRole);
}

void UMissionManagerSubsystem::GetPerPlayerObjectiveProgress(APlayerState* PlayerState, FName MissionID, int32 ObjectiveIndex, bool& bFound, FMissionObjectiveProgress& OutProgress) const
{
	bFound = false;
	const AMissionPlayerState* MPS = Cast<AMissionPlayerState>(PlayerState);
	if (!MPS) return;
	MPS->GetPerPlayerObjectiveProgress(MissionID, ObjectiveIndex, bFound, OutProgress);
}

TArray<FMissionSummaryForUI> UMissionManagerSubsystem::GetActiveMissionsForUI(APlayerState* ForPlayer) const
{
	TArray<FMissionSummaryForUI> Out;
	AMissionGameState* GS = GetOrFindMissionGameState();
	if (!GS) return Out;

	const AMissionPlayerState* MPS = ForPlayer ? Cast<AMissionPlayerState>(ForPlayer) : nullptr;

	for (const auto& Pair : MissionDataMap)
	{
		FName MissionID = Pair.Key;
		UMissionDataAsset* Data = Pair.Value;
		if (!Data) continue;
		if (GS->GetMissionState(MissionID) != EMissionState::Active) continue;

		FMissionSummaryForUI Summary;
		Summary.MissionID = MissionID;
		Summary.MissionDisplayName = Data->DisplayName.IsEmpty() ? FText::FromName(MissionID) : Data->DisplayName;
		Summary.Objectives.Reserve(Data->Objectives.Num());

		for (int32 i = 0; i < Data->Objectives.Num(); ++i)
		{
			const FMissionObjectiveDef& ObjDef = Data->Objectives[i];
			FMissionObjectiveSummaryForUI ObjUI;
			ObjUI.DisplayName = ObjDef.DisplayName.IsEmpty() ? FText::FromString(FString::Printf(TEXT("Objective %d"), i + 1)) : ObjDef.DisplayName;
			ObjUI.TargetCount = ObjDef.TargetCount;
			ObjUI.bIsPerPlayer = (ObjDef.ObjectiveScope == EObjectiveScope::PerPlayer);

			if (ObjDef.ObjectiveScope == EObjectiveScope::Session)
			{
				bool bFound = false;
				FMissionObjectiveProgress P;
				GS->GetObjectiveProgress(MissionID, i, bFound, P);
				if (bFound)
				{
					ObjUI.CurrentCount = P.CurrentCount;
					ObjUI.bCompleted = P.bCompleted;
				}
			}
			else
			{
				if (MPS)
				{
					bool bFound = false;
					FMissionObjectiveProgress P;
					MPS->GetPerPlayerObjectiveProgress(MissionID, i, bFound, P);
					if (bFound)
					{
						ObjUI.CurrentCount = P.CurrentCount;
						ObjUI.bCompleted = P.bCompleted;
					}
				}
			}
			// Formatted display: "Visit Waypoint" when target is 1, "Kill 5 monsters (0/5)" when target > 1 (no checkmark in text; use bCompleted for icon)
			if (ObjUI.TargetCount > 1)
			{
				ObjUI.FormattedDisplayText = FText::FromString(FString::Printf(TEXT("%s (%d/%d)"),
					*ObjUI.DisplayName.ToString(), ObjUI.CurrentCount, ObjUI.TargetCount));
			}
			else
			{
				ObjUI.FormattedDisplayText = ObjUI.DisplayName;
			}
			Summary.Objectives.Add(ObjUI);
		}
		Out.Add(Summary);
	}
	return Out;
}

bool UMissionManagerSubsystem::AreAllSessionObjectivesComplete(FName MissionID, const UMissionDataAsset* Data, AMissionGameState* GS) const
{
	if (!Data || !GS) return false;
	for (int32 i = 0; i < Data->Objectives.Num(); ++i)
	{
		if (Data->Objectives[i].ObjectiveScope == EObjectiveScope::PerPlayer) continue;
		bool bFound = false;
		FMissionObjectiveProgress P;
		GS->GetObjectiveProgress(MissionID, i, bFound, P);
		if (!bFound || !P.bCompleted) return false;
	}
	return true;
}

bool UMissionManagerSubsystem::AreAllPlayersPerPlayerObjectivesComplete(FName MissionID, const UMissionDataAsset* Data) const
{
	if (!Data) return false;
	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (!World) return false;
	AGameStateBase* GSB = World->GetGameState();
	if (!GSB) return false;
	bool bCountedAnyPlayer = false;
	for (APlayerState* PS : GSB->PlayerArray)
	{
		if (!PS || !PS->GetPawn()) continue; // only count players with pawns

		AMissionPlayerState* MPS = Cast<AMissionPlayerState>(PS);
		if (!MPS)
		{
			// If the session is using a non-mission PlayerState, we cannot validate per-player completion.
			// Treat as NOT complete to avoid incorrectly completing the mission early.
			return false;
		}

		bCountedAnyPlayer = true;
		if (!MPS->AreAllPerPlayerObjectivesCompleteForMission(MissionID, Data)) return false;
	}
	return bCountedAnyPlayer;
}

void UMissionManagerSubsystem::SetMissionState(FName MissionID, EMissionState NewState)
{
	if (!HasAuthority()) return;

	UMissionDataAsset* Data = GetMissionData(MissionID);
	EnsureMissionStateInReplicatedArrays(MissionID, NewState);

	const bool bCompleted = (NewState == EMissionState::Completed_Success || NewState == EMissionState::Completed_Failure || NewState == EMissionState::Expired);
		if (bCompleted && Data)
		{
			UE_LOG(LogTemp, Log, TEXT("MissionManager: Broadcasting OnMissionCompleted %s (success=%d)"), *MissionID.ToString(), (NewState == EMissionState::Completed_Success));
			OnMissionCompleted.Broadcast(MissionID, (NewState == EMissionState::Completed_Success), NewState, Data);
			if (NewState == EMissionState::Completed_Success)
			{
				if (Data->ThreatDeltaOnSuccess != 0)
					AddThreat(Data->ThreatDeltaOnSuccess, TMap<FName, int32>());
				ApplyMissionActions(Data->OnSuccessActions, true);
			}
			else
			{
				if (Data->ThreatDeltaOnFailure != 0)
					AddThreat(Data->ThreatDeltaOnFailure, TMap<FName, int32>());
				ApplyMissionActions(Data->OnFailureActions, false);
			}
		}
		if (NewState == EMissionState::Active && Data)
		{
			UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
			if (World)
			{
				MissionActiveStartTime.Add(MissionID, World->GetTimeSeconds());
			}
			AMissionGameState* GS = GetOrFindMissionGameState();
		if (GS)
		{
			for (int32 i = 0; i < Data->Objectives.Num(); ++i)
			{
				if (Data->Objectives[i].ObjectiveScope == EObjectiveScope::PerPlayer) continue;
				bool bExists = false;
				for (const FMissionObjectiveProgress& P : GS->ObjectiveProgress)
				{
					if (P.MissionID == MissionID && P.ObjectiveIndex == i) { bExists = true; break; }
				}
				if (!bExists)
				{
					FMissionObjectiveProgress P;
					P.MissionID = MissionID;
					P.ObjectiveIndex = i;
					P.CurrentCount = 0;
					P.bCompleted = false;
					int32 TargetCount = Data->Objectives[i].TargetCount;
					if (Data->bRandomizeTargetCount && Data->MinTargetCount <= Data->MaxTargetCount)
						TargetCount = FMath::RandRange(Data->MinTargetCount, Data->MaxTargetCount);
					P.TimeRemaining = Data->Objectives[i].TimeLimitSeconds > 0.0f ? Data->Objectives[i].TimeLimitSeconds : 0.0f;
					GS->ObjectiveProgress.Add(P);
				}
			}
		}
		OnMissionActivated.Broadcast(MissionID, Data);
	}
}

void UMissionManagerSubsystem::AddThreat(int32 GlobalDelta, const TMap<FName, int32>& FactionDelta)
{
	if (!HasAuthority()) return;

	AMissionGameState* GS = GetOrFindMissionGameState();
	if (!GS) return;

	int32 Previous = GS->GlobalThreatLevel;
	GS->GlobalThreatLevel = FMath::Max(0, GS->GlobalThreatLevel + GlobalDelta);
	for (const auto& Pair : FactionDelta)
	{
		int32 Current = GS->GetFactionThreatLevel(Pair.Key);
		int32 NewLevel = FMath::Max(0, Current + Pair.Value);
		EnsureFactionThreatInReplicatedArrays(Pair.Key, NewLevel);
	}

	if (Previous != GS->GlobalThreatLevel)
	{
		OnThreatChanged.Broadcast(GS->GlobalThreatLevel, Previous);
		CheckThreatThresholds(Previous, GS->GlobalThreatLevel);
	}
	LastBroadcastThreatLevel = GS->GlobalThreatLevel;
}

void UMissionManagerSubsystem::EvaluateVisibilityConditions()
{
	if (!HasAuthority()) return;

	AMissionGameState* GS = GetOrFindMissionGameState();
	if (!GS) return;

	for (const auto& Pair : MissionDataMap)
	{
		FName MissionID = Pair.Key;
		UMissionDataAsset* Data = Pair.Value;
		if (GS->GetMissionState(MissionID) != EMissionState::Hidden) continue;
		if (!EvaluateAllVisibilityConditions(Data)) continue;

		EnsureMissionStateInReplicatedArrays(MissionID, EMissionState::Available);
	}
}

void UMissionManagerSubsystem::Tick(float DeltaTime)
{
	if (!HasAuthority()) return;

	SessionTimeSeconds += DeltaTime;
	EvaluateVisibilityConditions();

	AMissionGameState* GS = GetOrFindMissionGameState();
	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (!GS || !World) return;

	for (const auto& Pair : MissionDataMap)
	{
		FName MissionID = Pair.Key;
		UMissionDataAsset* Data = Pair.Value;
		if (GS->GetMissionState(MissionID) != EMissionState::Active) continue;

		// Mission-level time limit
		if (Data->TimeLimitSeconds > 0.0f)
		{
			const double* StartTime = MissionActiveStartTime.Find(MissionID);
			if (StartTime && (World->GetTimeSeconds() - *StartTime) >= static_cast<double>(Data->TimeLimitSeconds))
			{
				SetMissionState(MissionID, EMissionState::Expired);
				return;
			}
		}

		// Update objective time remaining
		for (int32 i = 0; i < GS->ObjectiveProgress.Num(); ++i)
		{
			if (GS->ObjectiveProgress[i].MissionID != MissionID || GS->ObjectiveProgress[i].bCompleted) continue;
			if (i >= Data->Objectives.Num()) continue;
			if (Data->Objectives[i].TimeLimitSeconds <= 0.0f) continue;

			GS->ObjectiveProgress[i].TimeRemaining -= DeltaTime;
			if (GS->ObjectiveProgress[i].TimeRemaining <= 0.0f)
			{
				// Objective time expired -> fail mission
				CompleteMission(MissionID, false);
				SetMissionState(MissionID, EMissionState::Completed_Failure);
				return;
			}
		}
	}
}

AMissionGameState* UMissionManagerSubsystem::GetOrFindMissionGameState() const
{
	UGameInstance* GI = GetGameInstance();
	if (!GI) return nullptr;

	UWorld* World = GI->GetWorld();
	if (!World) return nullptr;

	AGameStateBase* Base = World->GetGameState();
	AMissionGameState* GS = Cast<AMissionGameState>(Base);
	if (GS)
	{
		const_cast<UMissionManagerSubsystem*>(this)->CachedMissionGameState = GS;
	}
	return GS;
}

void UMissionManagerSubsystem::EnsureMissionStateInReplicatedArrays(FName MissionID, EMissionState State)
{
	AMissionGameState* GS = GetOrFindMissionGameState();
	if (!GS) return;

	int32 Index = -1;
	for (int32 i = 0; i < GS->MissionStates.Num(); ++i)
	{
		if (GS->MissionStates[i].MissionID == MissionID) { Index = i; break; }
	}
	if (Index >= 0)
	{
		GS->MissionStates[Index].State = static_cast<uint8>(State);
	}
	else
	{
		FReplicatedMissionState Entry;
		Entry.MissionID = MissionID;
		Entry.State = static_cast<uint8>(State);
		GS->MissionStates.Add(Entry);
	}
}

void UMissionManagerSubsystem::EnsureFactionThreatInReplicatedArrays(FName FactionID, int32 Level)
{
	AMissionGameState* GS = GetOrFindMissionGameState();
	if (!GS || FactionID.IsNone()) return;

	int32 Index = -1;
	for (int32 i = 0; i < GS->FactionThreatLevels.Num(); ++i)
	{
		if (GS->FactionThreatLevels[i].FactionID == FactionID) { Index = i; break; }
	}
	if (Index >= 0)
	{
		GS->FactionThreatLevels[Index].ThreatLevel = Level;
	}
	else
	{
		FReplicatedFactionThreat Entry;
		Entry.FactionID = FactionID;
		Entry.ThreatLevel = Level;
		GS->FactionThreatLevels.Add(Entry);
	}
}

void UMissionManagerSubsystem::ApplyMissionActions(const TArray<FMissionAction>& Actions, bool bSuccess)
{
	if (!HasAuthority()) return;

	for (const FMissionAction& Action : Actions)
	{
		if (Action.ThreatDelta != 0)
		{
			TMap<FName, int32> FactionDelta = Action.FactionThreatDelta;
			AddThreat(Action.ThreatDelta, FactionDelta);
		}
		if (!Action.TargetMissionID.IsNone())
		{
			EMissionState NewState = Action.bActivateMission ? EMissionState::Active : EMissionState::Available;
			EnsureMissionStateInReplicatedArrays(Action.TargetMissionID, NewState);
			if (Action.bActivateMission)
			{
				UMissionDataAsset* Data = GetMissionData(Action.TargetMissionID);
				if (Data)
				{
					UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
					if (World)
					{
						MissionActiveStartTime.Add(Action.TargetMissionID, World->GetTimeSeconds());
					}
					AMissionGameState* GS = GetOrFindMissionGameState();
					if (GS)
					{
						for (int32 i = 0; i < Data->Objectives.Num(); ++i)
						{
							if (Data->Objectives[i].ObjectiveScope == EObjectiveScope::PerPlayer) continue;
							bool bExists = false;
							for (const FMissionObjectiveProgress& P : GS->ObjectiveProgress)
							{
								if (P.MissionID == Action.TargetMissionID && P.ObjectiveIndex == i) { bExists = true; break; }
							}
							if (!bExists)
							{
								FMissionObjectiveProgress P;
								P.MissionID = Action.TargetMissionID;
								P.ObjectiveIndex = i;
								P.CurrentCount = 0;
								P.bCompleted = false;
								P.TimeRemaining = Data->Objectives[i].TimeLimitSeconds > 0.0f ? Data->Objectives[i].TimeLimitSeconds : 0.0f;
								GS->ObjectiveProgress.Add(P);
							}
						}
					}
					OnMissionActivated.Broadcast(Action.TargetMissionID, Data);
					UE_LOG(LogTemp, Log, TEXT("MissionManager: Fail-forward activated mission %s"), *Action.TargetMissionID.ToString());
				}
			}
		}
		if (!Action.WorldEventName.IsNone())
		{
			BroadcastWorldEvent(Action.WorldEventName);
		}
	}
}

void UMissionManagerSubsystem::CompleteMission(FName MissionID, bool bSuccess)
{
	UE_LOG(LogTemp, Log, TEXT("MissionManager: CompleteMission %s -> %s"), *MissionID.ToString(), bSuccess ? TEXT("Success") : TEXT("Failure"));
	EMissionState FinalState = bSuccess ? EMissionState::Completed_Success : EMissionState::Completed_Failure;
	SetMissionState(MissionID, FinalState);
}

bool UMissionManagerSubsystem::EvaluateVisibilityCondition(const FMissionVisibilityCondition& Cond) const
{
	if (!Cond.RequiredMissionID.IsNone())
	{
		EMissionState Current = GetMissionState(Cond.RequiredMissionID);
		if (Current != Cond.RequiredMissionState) return false;
	}
	int32 Threat = GetGlobalThreatLevel();
	if (Threat < Cond.ThreatLevelMin || Threat > Cond.ThreatLevelMax) return false;
	if (Cond.TimeElapsedSeconds > 0.0f && SessionTimeSeconds < Cond.TimeElapsedSeconds) return false;
	return true;
}

bool UMissionManagerSubsystem::EvaluateAllVisibilityConditions(const UMissionDataAsset* Mission) const
{
	if (!Mission) return false;
	if (Mission->VisibilityConditions.Num() == 0) return true;
	for (const FMissionVisibilityCondition& Cond : Mission->VisibilityConditions)
	{
		if (!EvaluateVisibilityCondition(Cond)) return false;
	}
	return true;
}

void UMissionManagerSubsystem::CheckThreatThresholds(int32 PreviousThreat, int32 NewThreat)
{
	for (int32 Threshold : ThreatThresholds)
	{
		bool bAbove = (PreviousThreat < Threshold && NewThreat >= Threshold);
		bool bBelow = (PreviousThreat >= Threshold && NewThreat < Threshold);
		if (bAbove || bBelow)
		{
			OnThreatThresholdCrossed.Broadcast(NewThreat, Threshold, bAbove);
		}
	}
}

void UMissionManagerSubsystem::BroadcastWorldEvent(FName EventName)
{
	OnWorldEvent.Broadcast(EventName);
}

bool UMissionManagerSubsystem::HasAuthority() const
{
	UGameInstance* GI = GetGameInstance();
	if (!GI) return true;
	UWorld* World = GI->GetWorld();
	if (!World) return true;
	return World->GetNetMode() != NM_Client;
}
