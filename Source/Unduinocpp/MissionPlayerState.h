// Mission Player State - Per-player objective progress (replicated)
// Set as DefaultPlayerStateClass on your GameMode (MissionGameModeBase does this) so waypoints can report per-player progress.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "MissionTypes.h"
#include "MissionPlayerState.generated.h"

/**
 * PlayerState that holds replicated per-player mission objective progress.
 * Only objectives with ObjectiveScope = PerPlayer are stored here.
 * Session-scoped objectives remain on MissionGameState as before.
 */
UCLASS()
class UNDUINOCPP_API AMissionPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	AMissionPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Per-player objective progress; replicated so client UI can read locally owned player's progress. */
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Mission")
	TArray<FMissionObjectiveProgress> PerPlayerObjectiveProgress;

	/** Server-only: apply delta for one per-player objective; creates row if needed. Returns new CurrentCount, or -1 if not applied. */
	UFUNCTION(BlueprintCallable, Category = "Mission|Server")
	int32 ServerApplyPerPlayerObjectiveProgress(FName MissionID, int32 ObjectiveIndex, int32 DeltaCount, int32 TargetCount, float TimeLimitSeconds);

	UFUNCTION(BlueprintPure, Category = "Mission")
	void GetPerPlayerObjectiveProgress(FName MissionID, int32 ObjectiveIndex, bool& bFound, FMissionObjectiveProgress& OutProgress) const;

	/** True if this player has completed the given per-player objective (CurrentCount >= TargetCount or bCompleted). */
	UFUNCTION(BlueprintPure, Category = "Mission")
	bool IsPerPlayerObjectiveComplete(FName MissionID, int32 ObjectiveIndex, int32 TargetCount) const;

	/** True if every PerPlayer-scoped objective for this mission is complete for this player (by TargetCount). */
	UFUNCTION(BlueprintPure, Category = "Mission")
	bool AreAllPerPlayerObjectivesCompleteForMission(FName MissionID, const class UMissionDataAsset* MissionData) const;
};
