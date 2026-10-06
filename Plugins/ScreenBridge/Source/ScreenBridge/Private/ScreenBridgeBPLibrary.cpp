// Copyright (c) 2026, Srivanth. All Rights Reserved.

#include "ScreenBridgeBPLibrary.h"
#include "Engine/World.h"
#include "Blueprint/UserWidget.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "Widgets/Layout/SBox.h"
#include "Engine/GameInstance.h"

struct FScreenBridgeWindow
{
	TSharedPtr<SWindow> Window;
	TWeakObjectPtr<UWorld> World;
};

static TMap<int32, FScreenBridgeWindow> GScreenBridgeWindows;
static int32 GNextWindowId = 1;

bool UScreenBridgeBPLibrary::CreateExternalWindow(UObject* WorldContextObject,
	TSubclassOf<UUserWidget> WidgetClass,
	FVector2D Size,
	bool bBorderless,
	int32& OutWindowId)
{
	OutWindowId = INDEX_NONE;

	if (!WorldContextObject || !WidgetClass)
	{
		return false;
	}

	UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		return false;
	}

	UUserWidget* Widget = CreateWidget<UUserWidget>(World, WidgetClass);
	if (!Widget)
	{
		return false;
	}

	if (!Widget->IsValidLowLevel())
	{
		return false;
	}

	TSharedRef<SWindow> NewWindow = SNew(SWindow)
		.Title(FText::FromString(TEXT("")))
		.ClientSize(Size)
		.SupportsMaximize(false)
		.SupportsMinimize(false)
		.HasCloseButton(false)
		.CreateTitleBar(false);

	TSharedRef<SWidget> WidgetContent = Widget->TakeWidget();
	NewWindow->SetContent(WidgetContent);

	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().AddWindow(NewWindow);
	}
	else
	{
		return false;
	}

	const int32 NewId = GNextWindowId++;
	GScreenBridgeWindows.Add(NewId, { NewWindow, World });
	OutWindowId = NewId;

	return true;
}


void UScreenBridgeBPLibrary::SetWindowPosition(int32 WindowId, FVector2D NewPosition)
{
	if (FScreenBridgeWindow* Entry = GScreenBridgeWindows.Find(WindowId))
	{
		if (Entry->Window)
		{
			Entry->Window->MoveWindowTo(NewPosition);
		}
	}
}

void UScreenBridgeBPLibrary::SetWindowSize(int32 WindowId, FVector2D NewSize)
{
	if (FScreenBridgeWindow* Entry = GScreenBridgeWindows.Find(WindowId))
	{
		if (Entry->Window)
		{
			Entry->Window->Resize(NewSize);
		}
	}
}

void UScreenBridgeBPLibrary::CloseWindowsForWorld(const UWorld* World)
{
	if (!World) return;
	for (auto It = GScreenBridgeWindows.CreateIterator(); It; ++It)
	{
		FScreenBridgeWindow& Entry = It.Value();
		if (Entry.World.Get() == World)
		{
			if (Entry.Window.IsValid() && FSlateApplication::IsInitialized())
			{
				FSlateApplication::Get().RequestDestroyWindow(Entry.Window.ToSharedRef());
			}
			It.RemoveCurrent();
		}
	}
}

void UScreenBridgeBPLibrary::CloseAllWindows()
{
	if (FSlateApplication::IsInitialized())
	{
		for (const TPair<int32, FScreenBridgeWindow>& Entry : GScreenBridgeWindows)
		{
			if (Entry.Value.Window.IsValid())
			{
				FSlateApplication::Get().RequestDestroyWindow(Entry.Value.Window.ToSharedRef());
			}
		}
	}
	GScreenBridgeWindows.Empty();
}
