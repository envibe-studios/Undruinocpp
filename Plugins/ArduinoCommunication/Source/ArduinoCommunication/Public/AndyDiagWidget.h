// Andy ESP32 Diagnostics - UMG widget base class

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AndyDiagTypes.h"
#include "AndyDiagWidget.generated.h"

class UTextBlock;
class UButton;
class UVerticalBox;
class UAndyDiagSubsystem;
class UAndyDiagNodeRowWidget;

/**
 * Roll-call diagnostics panel for all ship ESP32 nodes.
 *
 * Preferred Widget Blueprint layout:
 *   - VerticalBox_NodeList  (dynamic rows, one per roll-call node)
 *   - Text_Summary
 *   - Button_Refresh (optional)
 *
 * Legacy optional bindings (used when VerticalBox_NodeList is absent):
 *   Text_AndyStatus, Text_AndyFailed, Text_PortStatus, Text_PortFailed
 */
UCLASS(Abstract, Blueprintable, BlueprintType)
class ARDUINOCOMMUNICATION_API UAndyDiagWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UAndyDiagWidget(const FObjectInitializer& ObjectInitializer);

	/** ShipId to query/send diagnostics for (must match AndySerialSubsystem registration). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Andy|Diagnostics")
	FName ShipId = TEXT("ShipA");

	/** If true, auto-request diagnostics when the widget is shown. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Andy|Diagnostics")
	bool bAutoRefreshOnShow = true;

	/**
	 * Expected roll-call node IDs (firmware DIAG_NODE names).
	 * Defaults to all 10 ship nodes; edit to match your deployment.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Andy|Diagnostics|RollCall")
	TArray<FName> RollCallNodeIds;

	/** Row widget class for VerticalBox_NodeList (optional BP child for styling). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Andy|Diagnostics|RollCall")
	TSubclassOf<UAndyDiagNodeRowWidget> NodeRowWidgetClass;

	/** Request a fresh quick-diag from Andy. */
	UFUNCTION(BlueprintCallable, Category = "Andy|Diagnostics")
	void RequestQuickDiag();

	/** Refresh display from the latest stored report. */
	UFUNCTION(BlueprintCallable, Category = "Andy|Diagnostics")
	void RefreshFromLatestReport();

	/** Push a report into the UI (called by subsystem event or manually). */
	UFUNCTION(BlueprintCallable, Category = "Andy|Diagnostics")
	void ApplyReport(const FAndyDiagReport& Report);

	/** Blueprint hook for custom styling when a report arrives. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Andy|Diagnostics")
	void OnReportApplied(const FAndyDiagReport& Report);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> VerticalBox_NodeList;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Summary;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Refresh;

	// Legacy two-node layout (backward compatible)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_AndyStatus;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_AndyFailed;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_PortStatus;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_PortFailed;

	UFUNCTION()
	void HandleDiagnosticsUpdated(FName UpdatedShipId, FAndyDiagReport Report);

	UFUNCTION()
	void HandleRefreshClicked();

private:
	UPROPERTY()
	TObjectPtr<UAndyDiagSubsystem> DiagSubsystem;

	void RebuildNodeList(const FAndyDiagReport& Report);
	void ApplyLegacyTwoNodeLayout(const FAndyDiagReport& Report);

	void ApplyNodeToRow(
		UTextBlock* StatusText,
		UTextBlock* FailedText,
		const FAndyDiagNodeInfo& Node) const;

	void UpdateNodeRow(
		UTextBlock* StatusText,
		UTextBlock* FailedText,
		const FAndyDiagReport& Report,
		const FString& NodeName) const;

	static FLinearColor StatusColor(EAndyDiagStatus Status);
};
