// Andy ESP32 Diagnostics - Line parser implementation

#include "AndyDiagParser.h"

bool UAndyDiagParser::IsDiagLine(const FString& Line)
{
	const FString Trimmed = Line.TrimStartAndEnd();
	return Trimmed.StartsWith(TEXT("DIAG_"), ESearchCase::CaseSensitive);
}

EAndyDiagLineType UAndyDiagParser::ParseLine(
	const FString& Line,
	FAndyDiagNodeInfo& OutNode,
	FAndyDiagSummary& OutSummary,
	FString& OutDiagMode,
	int32& OutExpectedNodeCount)
{
	OutNode = FAndyDiagNodeInfo();
	OutSummary = FAndyDiagSummary();
	OutDiagMode.Empty();
	OutExpectedNodeCount = 0;

	const FString Trimmed = Line.TrimStartAndEnd();
	if (!Trimmed.StartsWith(TEXT("DIAG_"), ESearchCase::CaseSensitive))
	{
		return EAndyDiagLineType::Ignored;
	}

	const TArray<FString> Fields = SplitCsvFields(Trimmed);
	if (Fields.Num() == 0)
	{
		return EAndyDiagLineType::Ignored;
	}

	const FString& Tag = Fields[0];

	if (Tag.Equals(TEXT("DIAG_BEGIN"), ESearchCase::IgnoreCase))
	{
		if (Fields.Num() > 1)
		{
			OutDiagMode = Fields[1];
		}
		if (Fields.Num() > 2)
		{
			OutExpectedNodeCount = FCString::Atoi(*Fields[2]);
		}
		return EAndyDiagLineType::Begin;
	}

	if (Tag.Equals(TEXT("DIAG_END"), ESearchCase::IgnoreCase))
	{
		return EAndyDiagLineType::End;
	}

	if (Tag.Equals(TEXT("DIAG_NODE"), ESearchCase::IgnoreCase))
	{
		if (Fields.Num() < 6)
		{
			UE_LOG(LogTemp, Verbose, TEXT("AndyDiagParser: Malformed DIAG_NODE (expected 6 fields): %s"), *Trimmed);
			return EAndyDiagLineType::Ignored;
		}

		OutNode.NodeName = Fields[1];
		OutNode.Status = ParseStatus(Fields[2]);
		SplitPipeList(Fields[3], OutNode.PassedTests);
		SplitPipeList(Fields[4], OutNode.FailedTests);
		OutNode.Uptime = FCString::Atoi(*Fields[5]);
		return EAndyDiagLineType::Node;
	}

	if (Tag.Equals(TEXT("DIAG_SUMMARY"), ESearchCase::IgnoreCase))
	{
		if (Fields.Num() < 7)
		{
			UE_LOG(LogTemp, Verbose, TEXT("AndyDiagParser: Malformed DIAG_SUMMARY (expected 7 fields): %s"), *Trimmed);
			return EAndyDiagLineType::Ignored;
		}

		OutSummary.ExpectedCount = FCString::Atoi(*Fields[1]);
		OutSummary.OnlineCount = FCString::Atoi(*Fields[2]);
		OutSummary.OkCount = FCString::Atoi(*Fields[3]);
		OutSummary.WarnCount = FCString::Atoi(*Fields[4]);
		OutSummary.FailCount = FCString::Atoi(*Fields[5]);
		OutSummary.OfflineCount = FCString::Atoi(*Fields[6]);
		return EAndyDiagLineType::Summary;
	}

	return EAndyDiagLineType::Ignored;
}

FString UAndyDiagParser::StatusToString(EAndyDiagStatus Status)
{
	switch (Status)
	{
	case EAndyDiagStatus::OK:      return TEXT("OK");
	case EAndyDiagStatus::Warn:    return TEXT("WARN");
	case EAndyDiagStatus::Fail:    return TEXT("FAIL");
	case EAndyDiagStatus::Offline: return TEXT("OFFLINE");
	default:                       return TEXT("UNKNOWN");
	}
}

EAndyDiagStatus UAndyDiagParser::ParseStatus(const FString& StatusStr)
{
	if (StatusStr.Equals(TEXT("OK"), ESearchCase::IgnoreCase))
	{
		return EAndyDiagStatus::OK;
	}
	if (StatusStr.Equals(TEXT("WARN"), ESearchCase::IgnoreCase))
	{
		return EAndyDiagStatus::Warn;
	}
	if (StatusStr.Equals(TEXT("FAIL"), ESearchCase::IgnoreCase))
	{
		return EAndyDiagStatus::Fail;
	}
	if (StatusStr.Equals(TEXT("OFFLINE"), ESearchCase::IgnoreCase))
	{
		return EAndyDiagStatus::Offline;
	}
	return EAndyDiagStatus::Unknown;
}

FString UAndyDiagParser::FormatFailedTests(const FAndyDiagNodeInfo& Node)
{
	if (Node.FailedTests.Num() == 0)
	{
		return FString();
	}
	return FString::Printf(TEXT("Failed: %s"), *FString::Join(Node.FailedTests, TEXT(", ")));
}

FString UAndyDiagParser::FormatSummaryText(const FAndyDiagSummary& Summary)
{
	return Summary.GetSummaryText();
}

bool UAndyDiagParser::FindNodeInReport(const FAndyDiagReport& Report, const FString& NodeName, FAndyDiagNodeInfo& OutNode)
{
	if (const FAndyDiagNodeInfo* Found = Report.FindNode(NodeName))
	{
		OutNode = *Found;
		return true;
	}
	return false;
}

TArray<FString> UAndyDiagParser::SplitCsvFields(const FString& Line)
{
	TArray<FString> Fields;
	Line.ParseIntoArray(Fields, TEXT(","), false);
	for (FString& Field : Fields)
	{
		Field = Field.TrimStartAndEnd();
	}
	return Fields;
}

void UAndyDiagParser::SplitPipeList(const FString& Field, TArray<FString>& OutList)
{
	OutList.Empty();
	const FString Trimmed = Field.TrimStartAndEnd();
	if (Trimmed.IsEmpty() || Trimmed.Equals(TEXT("NONE"), ESearchCase::IgnoreCase))
	{
		return;
	}

	Trimmed.ParseIntoArray(OutList, TEXT("|"), true);
	for (FString& Item : OutList)
	{
		Item = Item.TrimStartAndEnd();
	}
	OutList.RemoveAll([](const FString& S) { return S.IsEmpty(); });
}
