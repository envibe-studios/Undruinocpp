// Andy ESP32 Diagnostics - Line parser

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AndyDiagTypes.h"
#include "AndyDiagParser.generated.h"

/**
 * Static parser for Andy DIAG_* serial lines.
 * Non-diagnostic lines are ignored and never modify state.
 */
UCLASS()
class ARDUINOCOMMUNICATION_API UAndyDiagParser : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Returns true if the line is a diagnostic line (starts with DIAG_ after trim). */
	UFUNCTION(BlueprintPure, Category = "Andy|Diagnostics")
	static bool IsDiagLine(const FString& Line);

	/**
	 * Parse a single serial line.
	 * @return Line type; OutNode/OutSummary are valid when type is Node/Summary.
	 */
	UFUNCTION(BlueprintCallable, Category = "Andy|Diagnostics")
	static EAndyDiagLineType ParseLine(
		const FString& Line,
		FAndyDiagNodeInfo& OutNode,
		FAndyDiagSummary& OutSummary,
		FString& OutDiagMode,
		int32& OutExpectedNodeCount
	);

	/** Convert status enum to display string */
	UFUNCTION(BlueprintPure, Category = "Andy|Diagnostics")
	static FString StatusToString(EAndyDiagStatus Status);

	/** Convert status string from firmware to enum */
	UFUNCTION(BlueprintPure, Category = "Andy|Diagnostics")
	static EAndyDiagStatus ParseStatus(const FString& StatusStr);

	/** Format failed tests for UI, or empty if none */
	UFUNCTION(BlueprintPure, Category = "Andy|Diagnostics")
	static FString FormatFailedTests(const FAndyDiagNodeInfo& Node);

	/** Format summary line for UI */
	UFUNCTION(BlueprintPure, Category = "Andy|Diagnostics")
	static FString FormatSummaryText(const FAndyDiagSummary& Summary);

	/** Find node in report by name (case-insensitive) */
	UFUNCTION(BlueprintPure, Category = "Andy|Diagnostics")
	static bool FindNodeInReport(const FAndyDiagReport& Report, const FString& NodeName, FAndyDiagNodeInfo& OutNode);

private:
	static TArray<FString> SplitCsvFields(const FString& Line);
	static void SplitPipeList(const FString& Field, TArray<FString>& OutList);
};
