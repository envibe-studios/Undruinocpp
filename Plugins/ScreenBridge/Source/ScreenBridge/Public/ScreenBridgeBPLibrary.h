// Copyright (c) 2026, Srivanth. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Blueprint/UserWidget.h"
#include "ScreenBridgeBPLibrary.generated.h"


UCLASS()
class UScreenBridgeBPLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "ScreenBridge", meta = (WorldContext = "WorldContextObject"))
	static bool CreateExternalWindow(UObject* WorldContextObject,
		TSubclassOf<UUserWidget> WidgetClass,
		FVector2D Size,
		bool bBorderless,
		int32& OutWindowId);

	UFUNCTION(BlueprintCallable, Category = "ScreenBridge")
	static void SetWindowPosition(int32 WindowId, FVector2D NewPosition);

	UFUNCTION(BlueprintCallable, Category = "ScreenBridge")
	static void SetWindowSize(int32 WindowId, FVector2D NewSize);
};
