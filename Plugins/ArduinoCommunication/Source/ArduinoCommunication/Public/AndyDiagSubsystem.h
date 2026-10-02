// Andy ESP32 Diagnostics - GameInstance subsystem

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AndyDiagTypes.h"
#include "AndyDiagSubsystem.generated.h"

class UAndySerialSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnAndyDiagnosticsUpdated,
	FName, ShipId,
	FAndyDiagReport, Report
);

/**
 * Manages Andy quick-diagnostics over serial.
 * Sends !diag,quick and accumulates DIAG_* lines into a report.
 *
 * Usage:
 *   1. Get Game Instance Subsystem -> AndyDiagSubsystem
 *   2. Call RequestQuickDiag("ShipA")  (or your ShipId)
 *   3. Bind to OnDiagnosticsUpdated
 */
UCLASS(BlueprintType)
class ARDUINOCOMMUNICATION_API UAndyDiagSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Send !diag,quick to Andy and reset the in-progress report for this ship. */
	UFUNCTION(BlueprintCallable, Category = "Andy|Diagnostics")
	bool RequestQuickDiag(FName ShipId);

	/** Process a raw serial line (only DIAG_* lines are handled). Returns true if consumed. */
	UFUNCTION(BlueprintCallable, Category = "Andy|Diagnostics")
	bool ProcessSerialLine(FName ShipId, const FString& Line);

	/** Get the latest completed report for a ship. */
	UFUNCTION(BlueprintPure, Category = "Andy|Diagnostics")
	FAndyDiagReport GetLatestReport(FName ShipId) const;

	/** Get the in-progress report (while between DIAG_BEGIN and DIAG_END). */
	UFUNCTION(BlueprintPure, Category = "Andy|Diagnostics")
	FAndyDiagReport GetInProgressReport(FName ShipId) const;

	/**
	 * Latest report merged with the default 10-node roll call (missing nodes = OFFLINE).
	 * Use for UI lists that always show every expected ESP32.
	 */
	UFUNCTION(BlueprintPure, Category = "Andy|Diagnostics|RollCall")
	TArray<FAndyDiagNodeInfo> GetOrderedRollCall(FName ShipId, const TArray<FName>& ExpectedNodeIds) const;

	/** Default 10-node roll call IDs for this project. */
	UFUNCTION(BlueprintPure, Category = "Andy|Diagnostics|RollCall")
	static TArray<FName> GetDefaultRollCallNodeIds();

	/** True while waiting for DIAG_END after a RequestQuickDiag. */
	UFUNCTION(BlueprintPure, Category = "Andy|Diagnostics")
	bool IsDiagInProgress(FName ShipId) const;

	/** Fired whenever diagnostic data changes (progress and final result). */
	UPROPERTY(BlueprintAssignable, Category = "Andy|Diagnostics|Events")
	FOnAndyDiagnosticsUpdated OnDiagnosticsUpdated;

protected:
	UFUNCTION()
	void HandleSerialLine(FName ShipId, const FString& Line);

	void ResetInProgressReport(FName ShipId);
	void UpsertNode(FAndyDiagReport& Report, const FAndyDiagNodeInfo& Node);
	void BroadcastProgress(FName ShipId, const FAndyDiagReport& Report);

	/** Resolve and bind the serial subsystem if not already bound. Returns true if available. */
	bool EnsureSerialSubsystem();

private:
	UPROPERTY()
	TObjectPtr<UAndySerialSubsystem> SerialSubsystem;

	/** Latest completed report per ship */
	UPROPERTY()
	TMap<FName, FAndyDiagReport> LatestReports;

	/** In-progress report per ship (between BEGIN and END) */
	UPROPERTY()
	TMap<FName, FAndyDiagReport> InProgressReports;
};
