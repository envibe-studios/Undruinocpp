// Andy ESP32 Diagnostics - UMG widget implementation

#include "AndyDiagWidget.h"
#include "AndyDiagNodeRowWidget.h"
#include "AndyDiagParser.h"
#include "AndyDiagRegistry.h"
#include "AndyDiagSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Engine/GameInstance.h"

UAndyDiagWidget::UAndyDiagWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	RollCallNodeIds = UAndyDiagRegistry::GetDefaultRollCallNodeIds();
}

void UAndyDiagWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (RollCallNodeIds.IsEmpty())
	{
		RollCallNodeIds = UAndyDiagRegistry::GetDefaultRollCallNodeIds();
	}

	if (UGameInstance* GI = GetGameInstance())
	{
		DiagSubsystem = GI->GetSubsystem<UAndyDiagSubsystem>();
		if (DiagSubsystem)
		{
			DiagSubsystem->OnDiagnosticsUpdated.AddDynamic(this, &UAndyDiagWidget::HandleDiagnosticsUpdated);
		}
	}

	if (Button_Refresh)
	{
		Button_Refresh->OnClicked.AddDynamic(this, &UAndyDiagWidget::HandleRefreshClicked);
	}

	if (bAutoRefreshOnShow)
	{
		RequestQuickDiag();
	}
	else
	{
		RefreshFromLatestReport();
	}
}

void UAndyDiagWidget::NativeDestruct()
{
	if (DiagSubsystem)
	{
		DiagSubsystem->OnDiagnosticsUpdated.RemoveDynamic(this, &UAndyDiagWidget::HandleDiagnosticsUpdated);
	}

	if (Button_Refresh)
	{
		Button_Refresh->OnClicked.RemoveDynamic(this, &UAndyDiagWidget::HandleRefreshClicked);
	}

	Super::NativeDestruct();
}

void UAndyDiagWidget::RequestQuickDiag()
{
	if (DiagSubsystem)
	{
		DiagSubsystem->RequestQuickDiag(ShipId);

		// Keep showing the last completed report while the new roll call runs.
		FAndyDiagReport Baseline = DiagSubsystem->GetLatestReport(ShipId);
		Baseline.bInProgress = true;
		if (Baseline.Summary.ExpectedCount == 0)
		{
			Baseline.Summary.ExpectedCount = RollCallNodeIds.Num();
		}
		ApplyReport(Baseline);
	}
}

void UAndyDiagWidget::RefreshFromLatestReport()
{
	if (DiagSubsystem)
	{
		ApplyReport(DiagSubsystem->GetLatestReport(ShipId));
	}
}

void UAndyDiagWidget::ApplyReport(const FAndyDiagReport& Report)
{
	if (VerticalBox_NodeList)
	{
		RebuildNodeList(Report);
	}
	else
	{
		ApplyLegacyTwoNodeLayout(Report);
	}

	if (Text_Summary)
	{
		if (Report.bHasSummary)
		{
			Text_Summary->SetText(FText::FromString(UAndyDiagParser::FormatSummaryText(Report.Summary)));
		}
		else if (Report.bInProgress)
		{
			const int32 Expected = Report.Summary.ExpectedCount > 0
				? Report.Summary.ExpectedCount
				: RollCallNodeIds.Num();
			Text_Summary->SetText(FText::FromString(
				FString::Printf(TEXT("Running diagnostics... (%d expected)"), Expected)));
		}
		else
		{
			Text_Summary->SetText(FText::FromString(TEXT("No diagnostic data")));
		}
	}

	OnReportApplied(Report);
}

void UAndyDiagWidget::RebuildNodeList(const FAndyDiagReport& Report)
{
	if (!VerticalBox_NodeList)
	{
		return;
	}

	VerticalBox_NodeList->ClearChildren();

	FAndyDiagReport StickyReport;
	if (Report.bInProgress && DiagSubsystem)
	{
		StickyReport = DiagSubsystem->GetLatestReport(ShipId);
	}

	const TArray<FAndyDiagNodeInfo> OrderedNodes =
		UAndyDiagRegistry::BuildOrderedRollCall(Report, RollCallNodeIds, StickyReport);

	for (const FAndyDiagNodeInfo& Node : OrderedNodes)
	{
		if (NodeRowWidgetClass)
		{
			UAndyDiagNodeRowWidget* Row = CreateWidget<UAndyDiagNodeRowWidget>(this, NodeRowWidgetClass);
			if (Row)
			{
				Row->SetNodeInfo(Node);
				VerticalBox_NodeList->AddChild(Row);
				continue;
			}
		}

		// Default: simple text row (no row Blueprint required).
		if (!WidgetTree)
		{
			continue;
		}

		UTextBlock* StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		if (!StatusText)
		{
			continue;
		}

		StatusText->SetText(FText::FromString(UAndyDiagRegistry::FormatNodeStatusLine(Node)));
		StatusText->SetColorAndOpacity(UAndyDiagRegistry::GetStatusColor(Node.Status));
		VerticalBox_NodeList->AddChild(StatusText);

		const FString FailedLine = UAndyDiagParser::FormatFailedTests(Node);
		if (!FailedLine.IsEmpty())
		{
			UTextBlock* FailedText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			if (FailedText)
			{
				FailedText->SetText(FText::FromString(FailedLine));
				FailedText->SetColorAndOpacity(FLinearColor(1.0f, 0.4f, 0.4f));
				VerticalBox_NodeList->AddChild(FailedText);
			}
		}
	}
}

void UAndyDiagWidget::ApplyLegacyTwoNodeLayout(const FAndyDiagReport& Report)
{
	FAndyDiagNodeInfo AndyNode;
	FAndyDiagNodeInfo PortNode;
	const bool bHasAndy = UAndyDiagParser::FindNodeInReport(Report, TEXT("ANDY"), AndyNode);
	const bool bHasPort = UAndyDiagParser::FindNodeInReport(Report, TEXT("PORT"), PortNode);

	if (bHasAndy)
	{
		ApplyNodeToRow(Text_AndyStatus, Text_AndyFailed, AndyNode);
	}
	else if (Report.Nodes.Num() > 0)
	{
		ApplyNodeToRow(Text_AndyStatus, Text_AndyFailed, Report.Nodes[0]);
	}
	else
	{
		UpdateNodeRow(Text_AndyStatus, Text_AndyFailed, Report, TEXT("ANDY"));
	}

	if (bHasPort)
	{
		ApplyNodeToRow(Text_PortStatus, Text_PortFailed, PortNode);
	}
	else if (Report.Nodes.Num() > 1)
	{
		ApplyNodeToRow(Text_PortStatus, Text_PortFailed, Report.Nodes[1]);
	}
	else
	{
		UpdateNodeRow(Text_PortStatus, Text_PortFailed, Report, TEXT("PORT"));
	}
}

void UAndyDiagWidget::HandleDiagnosticsUpdated(FName UpdatedShipId, FAndyDiagReport Report)
{
	if (UpdatedShipId != ShipId)
	{
		return;
	}
	ApplyReport(Report);
}

void UAndyDiagWidget::HandleRefreshClicked()
{
	RequestQuickDiag();
}

void UAndyDiagWidget::UpdateNodeRow(
	UTextBlock* StatusText,
	UTextBlock* FailedText,
	const FAndyDiagReport& Report,
	const FString& NodeName) const
{
	if (!StatusText && !FailedText)
	{
		return;
	}

	FAndyDiagNodeInfo Node;
	if (UAndyDiagParser::FindNodeInReport(Report, NodeName, Node))
	{
		ApplyNodeToRow(StatusText, FailedText, Node);
	}
	else
	{
		if (StatusText)
		{
			const FText Label = UAndyDiagRegistry::GetDisplayLabel(FName(*NodeName));
			StatusText->SetText(FText::FromString(FString::Printf(TEXT("%s: --"), *Label.ToString())));
			StatusText->SetColorAndOpacity(FLinearColor::Gray);
		}
		if (FailedText)
		{
			FailedText->SetText(FText::GetEmpty());
			FailedText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UAndyDiagWidget::ApplyNodeToRow(
	UTextBlock* StatusText,
	UTextBlock* FailedText,
	const FAndyDiagNodeInfo& Node) const
{
	if (StatusText)
	{
		StatusText->SetText(FText::FromString(UAndyDiagRegistry::FormatNodeStatusLine(Node)));
		StatusText->SetColorAndOpacity(UAndyDiagRegistry::GetStatusColor(Node.Status));
	}

	if (FailedText)
	{
		const FString FailedLine = UAndyDiagParser::FormatFailedTests(Node);
		FailedText->SetText(FText::FromString(FailedLine));
		FailedText->SetVisibility(
			FailedLine.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
}

FLinearColor UAndyDiagWidget::StatusColor(EAndyDiagStatus Status)
{
	return UAndyDiagRegistry::GetStatusColor(Status);
}
