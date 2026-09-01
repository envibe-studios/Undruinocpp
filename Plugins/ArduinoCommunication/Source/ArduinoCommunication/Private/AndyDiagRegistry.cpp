// Andy ESP32 Diagnostics - Roll call node registry

#include "AndyDiagRegistry.h"
#include "AndyDiagParser.h"

namespace
{
	void RegisterAlias(TMap<FName, FName>& Map, FName Canonical, FName Alias)
	{
		Map.Add(Alias, Canonical);
	}

	const TMap<FName, FName>& BuildAliasToCanonicalMap()
	{
		static TMap<FName, FName> Map;
		if (Map.Num() == 0)
		{
			RegisterAlias(Map, TEXT("ANDY"), TEXT("ANDY"));

			RegisterAlias(Map, TEXT("PORT"), TEXT("PORT"));
			RegisterAlias(Map, TEXT("PORT"), TEXT("WEAPON_PORT"));

			RegisterAlias(Map, TEXT("WPN_STBD"), TEXT("WPN_STBD"));
			RegisterAlias(Map, TEXT("WPN_STBD"), TEXT("WEAPON_STARBOARD"));

			RegisterAlias(Map, TEXT("ENG_FL"), TEXT("ENG_FL"));
			RegisterAlias(Map, TEXT("ENG_FL"), TEXT("ENGINE_FRONT_LEFT"));

			RegisterAlias(Map, TEXT("ENG_FR"), TEXT("ENG_FR"));
			RegisterAlias(Map, TEXT("ENG_FR"), TEXT("ENGINE_FRONT_RIGHT"));

			RegisterAlias(Map, TEXT("ENG_RL"), TEXT("ENG_RL"));
			RegisterAlias(Map, TEXT("ENG_RL"), TEXT("ENGINE_REAR_LEFT"));

			RegisterAlias(Map, TEXT("ENG_RR"), TEXT("ENG_RR"));
			RegisterAlias(Map, TEXT("ENG_RR"), TEXT("ENGINE_REAR_RIGHT"));

			RegisterAlias(Map, TEXT("ENG_MAIN"), TEXT("ENG_MAIN"));
			RegisterAlias(Map, TEXT("ENG_MAIN"), TEXT("ENGINE_MAIN"));

			RegisterAlias(Map, TEXT("DISP_PORT"), TEXT("DISP_PORT"));
			RegisterAlias(Map, TEXT("DISP_PORT"), TEXT("WEAPON_PORT_DISPLAY"));

			RegisterAlias(Map, TEXT("DISP_STBD"), TEXT("DISP_STBD"));
			RegisterAlias(Map, TEXT("DISP_STBD"), TEXT("WEAPON_STARBOARD_DISPLAY"));
		}
		return Map;
	}

	const TArray<FAndyDiagNodeDefinition>& BuildDefaultDefinitions()
	{
		static const TArray<FAndyDiagNodeDefinition> Definitions = {
			{ TEXT("ANDY"),       NSLOCTEXT("AndyDiag", "Andy", "Andy"),                     0 },
			{ TEXT("PORT"),       NSLOCTEXT("AndyDiag", "WeaponPort", "Weapon Port"),         1 },
			{ TEXT("WPN_STBD"),   NSLOCTEXT("AndyDiag", "WeaponStbd", "Weapon Starboard"),    2 },
			{ TEXT("ENG_FL"),     NSLOCTEXT("AndyDiag", "EngFL", "Engine Front Left"),        3 },
			{ TEXT("ENG_FR"),     NSLOCTEXT("AndyDiag", "EngFR", "Engine Front Right"),       4 },
			{ TEXT("ENG_RL"),     NSLOCTEXT("AndyDiag", "EngRL", "Engine Rear Left"),         5 },
			{ TEXT("ENG_RR"),     NSLOCTEXT("AndyDiag", "EngRR", "Engine Rear Right"),        6 },
			{ TEXT("ENG_MAIN"),   NSLOCTEXT("AndyDiag", "EngMain", "Engine Main"),            7 },
			{ TEXT("DISP_PORT"),  NSLOCTEXT("AndyDiag", "DispPort", "Weapon Port Display"),   8 },
			{ TEXT("DISP_STBD"),  NSLOCTEXT("AndyDiag", "DispStbd", "Weapon Starboard Display"), 9 },
		};
		return Definitions;
	}
}

const TArray<FAndyDiagNodeDefinition>& UAndyDiagRegistry::GetDefaultDefinitions()
{
	return BuildDefaultDefinitions();
}

const TMap<FName, FName>& UAndyDiagRegistry::GetAliasToCanonicalMap()
{
	return BuildAliasToCanonicalMap();
}

TArray<FName> UAndyDiagRegistry::GetAliasesForCanonical(FName CanonicalId)
{
	TArray<FName> Aliases;
	for (const TPair<FName, FName>& Pair : GetAliasToCanonicalMap())
	{
		if (Pair.Value == CanonicalId)
		{
			Aliases.Add(Pair.Key);
		}
	}
	return Aliases;
}

FName UAndyDiagRegistry::CanonicalizeNodeId(FName RawNodeId)
{
	if (const FName* Canonical = GetAliasToCanonicalMap().Find(RawNodeId))
	{
		return *Canonical;
	}
	return RawNodeId;
}

bool UAndyDiagRegistry::IsKnownRollCallNode(FName NodeId)
{
	const FName Canonical = CanonicalizeNodeId(NodeId);
	for (const FAndyDiagNodeDefinition& Def : GetDefaultDefinitions())
	{
		if (Def.NodeId == Canonical)
		{
			return true;
		}
	}
	return false;
}

FAndyDiagNodeInfo UAndyDiagRegistry::NormalizeNodeInfo(const FAndyDiagNodeInfo& Node)
{
	FAndyDiagNodeInfo Normalized = Node;
	Normalized.NodeName = CanonicalizeNodeId(FName(*Node.NodeName)).ToString();
	return Normalized;
}

TArray<FAndyDiagNodeDefinition> UAndyDiagRegistry::GetDefaultRollCall()
{
	return GetDefaultDefinitions();
}

TArray<FName> UAndyDiagRegistry::GetDefaultRollCallNodeIds()
{
	TArray<FName> Ids;
	for (const FAndyDiagNodeDefinition& Def : GetDefaultDefinitions())
	{
		Ids.Add(Def.NodeId);
	}
	return Ids;
}

FText UAndyDiagRegistry::GetDisplayLabel(FName NodeId)
{
	const FName Canonical = CanonicalizeNodeId(NodeId);
	for (const FAndyDiagNodeDefinition& Def : GetDefaultDefinitions())
	{
		if (Def.NodeId == Canonical)
		{
			return Def.DisplayLabel;
		}
	}
	return FText::FromName(NodeId);
}

FAndyDiagNodeInfo UAndyDiagRegistry::MakeMissingNode(FName NodeId, EAndyDiagStatus Status)
{
	FAndyDiagNodeInfo Node;
	Node.NodeName = CanonicalizeNodeId(NodeId).ToString();
	Node.Status = Status;
	return Node;
}

bool UAndyDiagRegistry::FindNodeForRollCall(
	const FAndyDiagReport& Report,
	FName CanonicalId,
	FAndyDiagNodeInfo& OutNode)
{
	for (const FName& Alias : GetAliasesForCanonical(CanonicalId))
	{
		if (UAndyDiagParser::FindNodeInReport(Report, Alias.ToString(), OutNode))
		{
			OutNode = NormalizeNodeInfo(OutNode);
			return true;
		}
	}
	return false;
}

TArray<FAndyDiagNodeInfo> UAndyDiagRegistry::BuildOrderedRollCall(
	const FAndyDiagReport& Report,
	const TArray<FName>& ExpectedNodeIds,
	const FAndyDiagReport& StickyReport)
{
	TArray<FAndyDiagNodeInfo> Ordered;
	Ordered.Reserve(ExpectedNodeIds.Num());

	TSet<FName> ConsumedCanonicalIds;

	for (const FName& ExpectedId : ExpectedNodeIds)
	{
		const FName CanonicalId = CanonicalizeNodeId(ExpectedId);
		FAndyDiagNodeInfo NodeInfo;

		if (FindNodeForRollCall(Report, CanonicalId, NodeInfo))
		{
			Ordered.Add(NodeInfo);
			ConsumedCanonicalIds.Add(CanonicalId);
		}
		else if (Report.bInProgress && FindNodeForRollCall(StickyReport, CanonicalId, NodeInfo))
		{
			// Keep last-known status while a refresh is in flight (avoids "--" flicker).
			Ordered.Add(NodeInfo);
			ConsumedCanonicalIds.Add(CanonicalId);
		}
		else
		{
			const EAndyDiagStatus MissingStatus = Report.bInProgress
				? EAndyDiagStatus::Unknown
				: EAndyDiagStatus::Offline;
			Ordered.Add(MakeMissingNode(CanonicalId, MissingStatus));
		}
	}

	// Append truly unknown nodes only (not aliases of expected roll-call entries).
	for (const FAndyDiagNodeInfo& Node : Report.Nodes)
	{
		const FName CanonicalId = CanonicalizeNodeId(FName(*Node.NodeName));
		if (ConsumedCanonicalIds.Contains(CanonicalId))
		{
			continue;
		}
		if (IsKnownRollCallNode(CanonicalId))
		{
			continue;
		}

		Ordered.Add(NormalizeNodeInfo(Node));
		ConsumedCanonicalIds.Add(CanonicalId);
	}

	return Ordered;
}

FAndyDiagReport UAndyDiagRegistry::MergeWithPreviousReport(
	const FAndyDiagReport& NewReport,
	const FAndyDiagReport& PreviousReport,
	const TArray<FName>& ExpectedNodeIds)
{
	if (PreviousReport.Nodes.Num() == 0)
	{
		return NewReport;
	}

	FAndyDiagReport Merged = NewReport;
	Merged.Nodes.Empty();

	for (const FName& ExpectedId : ExpectedNodeIds)
	{
		const FName CanonicalId = CanonicalizeNodeId(ExpectedId);
		FAndyDiagNodeInfo NodeInfo;

		if (FindNodeForRollCall(NewReport, CanonicalId, NodeInfo))
		{
			Merged.Nodes.Add(NodeInfo);
		}
		else if (FindNodeForRollCall(PreviousReport, CanonicalId, NodeInfo))
		{
			Merged.Nodes.Add(NodeInfo);
		}
	}

	// Preserve unexpected extra nodes from the new report.
	TSet<FName> Consumed;
	for (const FAndyDiagNodeInfo& Node : Merged.Nodes)
	{
		Consumed.Add(CanonicalizeNodeId(FName(*Node.NodeName)));
	}
	for (const FAndyDiagNodeInfo& Node : NewReport.Nodes)
	{
		const FName CanonicalId = CanonicalizeNodeId(FName(*Node.NodeName));
		if (!Consumed.Contains(CanonicalId))
		{
			Merged.Nodes.Add(NormalizeNodeInfo(Node));
			Consumed.Add(CanonicalId);
		}
	}

	return Merged;
}

FString UAndyDiagRegistry::FormatNodeStatusLine(const FAndyDiagNodeInfo& Node)
{
	const FText Label = GetDisplayLabel(FName(*Node.NodeName));

	if (Node.Status == EAndyDiagStatus::Unknown)
	{
		return FString::Printf(TEXT("%s: ..."), *Label.ToString());
	}

	return FString::Printf(
		TEXT("%s: %s"),
		*Label.ToString(),
		*UAndyDiagParser::StatusToString(Node.Status));
}

FLinearColor UAndyDiagRegistry::GetStatusColor(EAndyDiagStatus Status)
{
	switch (Status)
	{
	case EAndyDiagStatus::OK:      return FLinearColor(0.2f, 0.9f, 0.3f);
	case EAndyDiagStatus::Warn:    return FLinearColor(1.0f, 0.85f, 0.2f);
	case EAndyDiagStatus::Fail:    return FLinearColor(1.0f, 0.25f, 0.25f);
	case EAndyDiagStatus::Offline: return FLinearColor(0.5f, 0.5f, 0.5f);
	default:                       return FLinearColor(0.7f, 0.7f, 0.7f);
	}
}
