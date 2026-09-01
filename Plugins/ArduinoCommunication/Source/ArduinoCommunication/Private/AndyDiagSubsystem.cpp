// Andy ESP32 Diagnostics - GameInstance subsystem implementation

#include "AndyDiagSubsystem.h"
#include "AndyDiagParser.h"
#include "AndyDiagRegistry.h"
#include "AndySerialSubsystem.h"
#include "Engine/GameInstance.h"

void UAndyDiagSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Guarantee the serial subsystem is created/initialized before we bind to it.
	// Without this, subsystem init order is undefined and GetSubsystem may return null.
	Collection.InitializeDependency(UAndySerialSubsystem::StaticClass());

	EnsureSerialSubsystem();
}

bool UAndyDiagSubsystem::EnsureSerialSubsystem()
{
	if (SerialSubsystem)
	{
		return true;
	}

	UGameInstance* GI = GetGameInstance();
	if (!GI)
	{
		return false;
	}

	SerialSubsystem = GI->GetSubsystem<UAndySerialSubsystem>();
	if (!SerialSubsystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("AndyDiagSubsystem: AndySerialSubsystem not available"));
		return false;
	}

	SerialSubsystem->OnLineReceived.AddDynamic(this, &UAndyDiagSubsystem::HandleSerialLine);
	UE_LOG(LogTemp, Log, TEXT("AndyDiagSubsystem: Bound to AndySerialSubsystem line events"));
	return true;
}

void UAndyDiagSubsystem::Deinitialize()
{
	if (SerialSubsystem)
	{
		SerialSubsystem->OnLineReceived.RemoveDynamic(this, &UAndyDiagSubsystem::HandleSerialLine);
		SerialSubsystem = nullptr;
	}

	LatestReports.Empty();
	InProgressReports.Empty();

	Super::Deinitialize();
}

bool UAndyDiagSubsystem::RequestQuickDiag(FName ShipId)
{
	if (!EnsureSerialSubsystem())
	{
		UE_LOG(LogTemp, Warning, TEXT("AndyDiagSubsystem: Cannot request diag - serial subsystem missing"));
		return false;
	}

	ResetInProgressReport(ShipId);

	const bool bSent = SerialSubsystem->SendLine(ShipId, TEXT("!diag,quick"));
	if (bSent)
	{
		FAndyDiagReport& Report = InProgressReports.FindOrAdd(ShipId);
		Report.bInProgress = true;
		UE_LOG(LogTemp, Log, TEXT("AndyDiagSubsystem: Sent !diag,quick to ShipId '%s'"), *ShipId.ToString());
	}
	else
	{
		// Report what is actually registered so a ShipId mismatch or unopened port is obvious.
		const TArray<FName> ShipIds = SerialSubsystem->GetAllShipIds();
		FString ShipList;
		for (const FName& Id : ShipIds)
		{
			ShipList += FString::Printf(TEXT("'%s'(connected=%s) "),
				*Id.ToString(),
				SerialSubsystem->IsConnected(Id) ? TEXT("true") : TEXT("false"));
		}
		if (ShipList.IsEmpty())
		{
			ShipList = TEXT("<none registered - did GameMode call AddPort + StartAll?>");
		}

		UE_LOG(LogTemp, Warning,
			TEXT("AndyDiagSubsystem: Failed to send !diag,quick to ShipId '%s'. Registered ships: %s"),
			*ShipId.ToString(), *ShipList);
		InProgressReports.Remove(ShipId);
	}

	return bSent;
}

bool UAndyDiagSubsystem::ProcessSerialLine(FName ShipId, const FString& RawLine)
{
	// Binary telemetry packets share this serial link, so a DIAG_ line can arrive
	// with non-text bytes glued to the front (the line won't START with "DIAG_").
	// Locate the DIAG_ marker anywhere in the line and parse from there. Trailing
	// binary cannot appear because the line was split on '\n' which terminates the
	// DIAG_ text. Non-diagnostic / binary lines simply won't contain the marker.
	const int32 MarkerIndex = RawLine.Find(TEXT("DIAG_"), ESearchCase::CaseSensitive, ESearchDir::FromStart);
	if (MarkerIndex == INDEX_NONE)
	{
		return false;
	}

	const FString Line = RawLine.RightChop(MarkerIndex);

	// Raw view of every diagnostic line (Verbose so it stays quiet by default).
	UE_LOG(LogTemp, Verbose, TEXT("AndyDiag RAW <%s>: [%s]%s"),
		*ShipId.ToString(),
		*Line,
		MarkerIndex > 0 ? *FString::Printf(TEXT(" (stripped %d leading binary byte(s))"), MarkerIndex) : TEXT(""));

	FAndyDiagNodeInfo Node;
	FAndyDiagSummary Summary;
	FString DiagMode;
	int32 ExpectedNodeCount = 0;

	const EAndyDiagLineType LineType = UAndyDiagParser::ParseLine(Line, Node, Summary, DiagMode, ExpectedNodeCount);

	if (LineType == EAndyDiagLineType::Ignored)
	{
		// Starts with DIAG_ but failed to parse - almost certainly malformed (wrong field count).
		UE_LOG(LogTemp, Warning, TEXT("AndyDiag: DIAG_ line could not be parsed (malformed/field count?): [%s]"), *Line);
		return false;
	}

	if (LineType == EAndyDiagLineType::Node)
	{
		UE_LOG(LogTemp, Log, TEXT("AndyDiag: Parsed NODE name='%s' status=%s passed=%d failed=%d uptime=%d"),
			*Node.NodeName,
			*UAndyDiagParser::StatusToString(Node.Status),
			Node.PassedTests.Num(),
			Node.FailedTests.Num(),
			Node.Uptime);
	}

	FAndyDiagReport& Report = InProgressReports.FindOrAdd(ShipId);

	switch (LineType)
	{
	case EAndyDiagLineType::Begin:
		// Only reset on BEGIN if nothing has been collected yet. Some firmware sends
		// DIAG_BEGIN after nodes (or twice), which would wipe ANDY/PORT from the report
		// while leaving DIAG_SUMMARY intact — summary updates, status rows stay "--".
		if (Report.Nodes.Num() == 0 && !Report.bHasSummary)
		{
			Report = FAndyDiagReport();
		}
		Report.bInProgress = true;
		if (ExpectedNodeCount > 0)
		{
			Report.Summary.ExpectedCount = ExpectedNodeCount;
		}
		BroadcastProgress(ShipId, Report);
		break;

	case EAndyDiagLineType::Node:
		UpsertNode(Report, Node);
		BroadcastProgress(ShipId, Report);
		break;

	case EAndyDiagLineType::Summary:
		Report.Summary = Summary;
		Report.bHasSummary = true;
		BroadcastProgress(ShipId, Report);
		break;

	case EAndyDiagLineType::End:
	{
		Report.bInProgress = false;

		FAndyDiagReport Finalized = Report;

		// Serial/binary traffic can drop individual DIAG_NODE lines. If this run
		// returned fewer nodes than the last good report, carry forward the gaps.
		if (const FAndyDiagReport* Previous = LatestReports.Find(ShipId))
		{
			if (Previous->Nodes.Num() > 0 && Finalized.Nodes.Num() < Previous->Nodes.Num())
			{
				UE_LOG(LogTemp, Verbose,
					TEXT("AndyDiagSubsystem: Incomplete roll call (%d nodes vs %d previously) — merging last-known nodes for '%s'"),
					Finalized.Nodes.Num(), Previous->Nodes.Num(), *ShipId.ToString());
				Finalized = UAndyDiagRegistry::MergeWithPreviousReport(
					Finalized, *Previous, UAndyDiagRegistry::GetDefaultRollCallNodeIds());
			}
		}

		LatestReports.Add(ShipId, Finalized);
		InProgressReports.Remove(ShipId);

		OnDiagnosticsUpdated.Broadcast(ShipId, Finalized);

		FString NodeList;
		for (const FAndyDiagNodeInfo& N : Finalized.Nodes)
		{
			NodeList += FString::Printf(TEXT("'%s'(%s) "),
				*N.NodeName,
				*UAndyDiagParser::StatusToString(N.Status));
		}
		if (NodeList.IsEmpty())
		{
			NodeList = TEXT("<none>");
		}

		UE_LOG(LogTemp, Log, TEXT("AndyDiagSubsystem: Diag complete for '%s' - %d nodes [%s], %s"),
			*ShipId.ToString(), Finalized.Nodes.Num(), *NodeList, *Finalized.Summary.GetSummaryText());
		break;
	}
	}

	return true;
}

FAndyDiagReport UAndyDiagSubsystem::GetLatestReport(FName ShipId) const
{
	if (const FAndyDiagReport* Found = LatestReports.Find(ShipId))
	{
		return *Found;
	}
	return FAndyDiagReport();
}

FAndyDiagReport UAndyDiagSubsystem::GetInProgressReport(FName ShipId) const
{
	if (const FAndyDiagReport* Found = InProgressReports.Find(ShipId))
	{
		return *Found;
	}
	return FAndyDiagReport();
}

bool UAndyDiagSubsystem::IsDiagInProgress(FName ShipId) const
{
	if (const FAndyDiagReport* Found = InProgressReports.Find(ShipId))
	{
		return Found->bInProgress;
	}
	return false;
}

TArray<FAndyDiagNodeInfo> UAndyDiagSubsystem::GetOrderedRollCall(
	FName ShipId,
	const TArray<FName>& ExpectedNodeIds) const
{
	const FAndyDiagReport Report = GetLatestReport(ShipId);
	const TArray<FName>& Ids = ExpectedNodeIds.Num() > 0
		? ExpectedNodeIds
		: UAndyDiagRegistry::GetDefaultRollCallNodeIds();
	return UAndyDiagRegistry::BuildOrderedRollCall(Report, Ids, FAndyDiagReport());
}

TArray<FName> UAndyDiagSubsystem::GetDefaultRollCallNodeIds()
{
	return UAndyDiagRegistry::GetDefaultRollCallNodeIds();
}

void UAndyDiagSubsystem::HandleSerialLine(FName ShipId, const FString& Line)
{
	ProcessSerialLine(ShipId, Line);
}

void UAndyDiagSubsystem::ResetInProgressReport(FName ShipId)
{
	InProgressReports.Remove(ShipId);
}

void UAndyDiagSubsystem::UpsertNode(FAndyDiagReport& Report, const FAndyDiagNodeInfo& Node)
{
	const FAndyDiagNodeInfo Normalized = UAndyDiagRegistry::NormalizeNodeInfo(Node);

	for (FAndyDiagNodeInfo& Existing : Report.Nodes)
	{
		const FName ExistingCanonical = UAndyDiagRegistry::CanonicalizeNodeId(FName(*Existing.NodeName));
		const FName NewCanonical = UAndyDiagRegistry::CanonicalizeNodeId(FName(*Normalized.NodeName));
		if (ExistingCanonical == NewCanonical)
		{
			Existing = Normalized;
			return;
		}
	}
	Report.Nodes.Add(Normalized);
}

void UAndyDiagSubsystem::BroadcastProgress(FName ShipId, const FAndyDiagReport& Report)
{
	OnDiagnosticsUpdated.Broadcast(ShipId, Report);
}
