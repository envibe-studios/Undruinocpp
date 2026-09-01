// Andy ESP32 Diagnostics - Blueprint-friendly data types

#pragma once

#include "CoreMinimal.h"
#include "AndyDiagTypes.generated.h"

/** Health status reported by a diagnostic node */
UENUM(BlueprintType)
enum class EAndyDiagStatus : uint8
{
	Unknown UMETA(DisplayName = "Unknown"),
	OK      UMETA(DisplayName = "OK"),
	Warn    UMETA(DisplayName = "Warn"),
	Fail    UMETA(DisplayName = "Fail"),
	Offline UMETA(DisplayName = "Offline")
};

/** Parsed DIAG_NODE line */
USTRUCT(BlueprintType)
struct FAndyDiagNodeInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Andy|Diagnostics")
	FString NodeName;

	UPROPERTY(BlueprintReadOnly, Category = "Andy|Diagnostics")
	EAndyDiagStatus Status = EAndyDiagStatus::Unknown;

	UPROPERTY(BlueprintReadOnly, Category = "Andy|Diagnostics")
	TArray<FString> PassedTests;

	UPROPERTY(BlueprintReadOnly, Category = "Andy|Diagnostics")
	TArray<FString> FailedTests;

	UPROPERTY(BlueprintReadOnly, Category = "Andy|Diagnostics")
	int32 Uptime = 0;
};

/** Parsed DIAG_SUMMARY line */
USTRUCT(BlueprintType)
struct FAndyDiagSummary
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Andy|Diagnostics")
	int32 ExpectedCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Andy|Diagnostics")
	int32 OnlineCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Andy|Diagnostics")
	int32 OkCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Andy|Diagnostics")
	int32 WarnCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Andy|Diagnostics")
	int32 FailCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Andy|Diagnostics")
	int32 OfflineCount = 0;

	/** Returns text like "Online: 2 / 2" */
	FString GetSummaryText() const
	{
		return FString::Printf(TEXT("Online: %d / %d"), OnlineCount, ExpectedCount);
	}
};

/** Full diagnostic snapshot for one quick-diag run */
USTRUCT(BlueprintType)
struct FAndyDiagReport
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Andy|Diagnostics")
	TArray<FAndyDiagNodeInfo> Nodes;

	UPROPERTY(BlueprintReadOnly, Category = "Andy|Diagnostics")
	FAndyDiagSummary Summary;

	UPROPERTY(BlueprintReadOnly, Category = "Andy|Diagnostics")
	bool bHasSummary = false;

	UPROPERTY(BlueprintReadOnly, Category = "Andy|Diagnostics")
	bool bInProgress = false;

	/** Find a node by name (case-insensitive). Returns nullptr if not found. */
	const FAndyDiagNodeInfo* FindNode(const FString& Name) const
	{
		for (const FAndyDiagNodeInfo& Node : Nodes)
		{
			if (Node.NodeName.Equals(Name, ESearchCase::IgnoreCase))
			{
				return &Node;
			}
		}
		return nullptr;
	}
};

/** Result of parsing a single serial line */
UENUM(BlueprintType)
enum class EAndyDiagLineType : uint8
{
	Ignored     UMETA(DisplayName = "Ignored"),
	Begin       UMETA(DisplayName = "DIAG_BEGIN"),
	Node        UMETA(DisplayName = "DIAG_NODE"),
	Summary     UMETA(DisplayName = "DIAG_SUMMARY"),
	End         UMETA(DisplayName = "DIAG_END")
};
