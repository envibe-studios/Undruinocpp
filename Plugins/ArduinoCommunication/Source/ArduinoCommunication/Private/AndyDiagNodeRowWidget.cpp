// Andy ESP32 Diagnostics - Single node row widget

#include "AndyDiagNodeRowWidget.h"
#include "AndyDiagParser.h"
#include "AndyDiagRegistry.h"
#include "Components/TextBlock.h"

void UAndyDiagNodeRowWidget::SetNodeInfo(const FAndyDiagNodeInfo& Node)
{
	if (Text_Status)
	{
		Text_Status->SetText(FText::FromString(UAndyDiagRegistry::FormatNodeStatusLine(Node)));
		Text_Status->SetColorAndOpacity(UAndyDiagRegistry::GetStatusColor(Node.Status));
	}

	if (Text_Failed)
	{
		const FString FailedLine = UAndyDiagParser::FormatFailedTests(Node);
		Text_Failed->SetText(FText::FromString(FailedLine));
		Text_Failed->SetVisibility(
			FailedLine.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
}
