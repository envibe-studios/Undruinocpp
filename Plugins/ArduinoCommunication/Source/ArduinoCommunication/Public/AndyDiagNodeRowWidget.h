// Andy ESP32 Diagnostics - Single node row widget

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AndyDiagTypes.h"
#include "AndyDiagNodeRowWidget.generated.h"

class UTextBlock;

/**
 * One row in the diagnostics roll-call list.
 * Create WBP_DiagNodeRow (optional) child for custom styling.
 */
UCLASS(Abstract, Blueprintable, BlueprintType)
class ARDUINOCOMMUNICATION_API UAndyDiagNodeRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Andy|Diagnostics")
	void SetNodeInfo(const FAndyDiagNodeInfo& Node);

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Status;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Failed;
};
