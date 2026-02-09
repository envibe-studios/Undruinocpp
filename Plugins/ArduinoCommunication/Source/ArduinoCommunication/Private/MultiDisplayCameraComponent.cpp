// Multi-Display Camera Component - Implementation

#include "MultiDisplayCameraComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Slate/SceneViewport.h"
#include "Widgets/SWindow.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/SViewport.h"
#include "Framework/Application/SlateApplication.h"
#include "Slate/SlateTextures.h"
#include "RHI.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Kismet/KismetRenderingLibrary.h"

UMultiDisplayCameraComponent::UMultiDisplayCameraComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	// Default scene capture settings
	bCaptureEveryFrame = true;
	bCaptureOnMovement = false;
	bAlwaysPersistRenderingState = true;

	// Use high quality capture settings
	CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
}

void UMultiDisplayCameraComponent::BeginPlay()
{
	Super::BeginPlay();

	// Setup render target
	SetupRenderTarget();

	// Auto-activate if configured
	if (bAutoActivate)
	{
		ActivateDisplay();
	}
}

void UMultiDisplayCameraComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Cleanup
	DeactivateDisplay();
	DestroySecondaryWindow();

	Super::EndPlay(EndPlayReason);
}

void UMultiDisplayCameraComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Handle frame rate limiting
	if (CaptureFrameRate > 0 && bIsDisplayActive)
	{
		CaptureTimer += DeltaTime;
		float FrameInterval = 1.0f / CaptureFrameRate;

		if (CaptureTimer >= FrameInterval)
		{
			CaptureTimer = 0.0f;
			// Scene capture will automatically update
		}
		else
		{
			// Skip capture this frame
			bCaptureEveryFrame = false;
		}
	}
	else
	{
		bCaptureEveryFrame = bIsDisplayActive;
	}
}

void UMultiDisplayCameraComponent::SetupRenderTarget()
{
	// Determine resolution
	int32 Width = RenderTargetWidth;
	int32 Height = RenderTargetHeight;

	// If resolution is 0, try to get display resolution
	if (Width == 0 || Height == 0)
	{
		FIntPoint DisplayRes;
		FString DisplayName;
		if (GetDisplayInfo(TargetDisplayIndex, DisplayName, DisplayRes))
		{
			Width = DisplayRes.X > 0 ? DisplayRes.X : 1920;
			Height = DisplayRes.Y > 0 ? DisplayRes.Y : 1080;
		}
		else
		{
			// Default resolution
			Width = 1920;
			Height = 1080;
		}
	}

	CachedDisplayResolution = FIntPoint(Width, Height);

	// Create or update render target
	if (!TextureTarget)
	{
		TextureTarget = NewObject<UTextureRenderTarget2D>(this);
	}

	TextureTarget->InitAutoFormat(Width, Height);
	TextureTarget->UpdateResourceImmediate(true);

	UE_LOG(LogTemp, Log, TEXT("MultiDisplayCamera: Created render target %dx%d for display %d"), Width, Height, TargetDisplayIndex);
}

void UMultiDisplayCameraComponent::ActivateDisplay()
{
	if (bIsDisplayActive)
	{
		return;
	}

	// Make sure render target is ready
	if (!TextureTarget)
	{
		SetupRenderTarget();
	}

	// Create secondary window for the target display
	CreateSecondaryWindow();

	bIsDisplayActive = true;
	bCaptureEveryFrame = true;

	UE_LOG(LogTemp, Log, TEXT("MultiDisplayCamera: Activated on display %d"), TargetDisplayIndex);
}

void UMultiDisplayCameraComponent::DeactivateDisplay()
{
	if (!bIsDisplayActive)
	{
		return;
	}

	RemoveFromDisplay();
	bIsDisplayActive = false;
	bCaptureEveryFrame = false;

	UE_LOG(LogTemp, Log, TEXT("MultiDisplayCamera: Deactivated from display %d"), TargetDisplayIndex);
}

bool UMultiDisplayCameraComponent::IsDisplayActive() const
{
	return bIsDisplayActive;
}

void UMultiDisplayCameraComponent::SetTargetDisplay(int32 NewDisplayIndex)
{
	if (NewDisplayIndex == TargetDisplayIndex)
	{
		return;
	}

	bool bWasActive = bIsDisplayActive;

	// Deactivate from current display
	if (bIsDisplayActive)
	{
		DeactivateDisplay();
	}

	// Update target
	TargetDisplayIndex = FMath::Clamp(NewDisplayIndex, 0, 7);

	// Refresh render target for new display resolution
	SetupRenderTarget();

	// Reactivate on new display
	if (bWasActive)
	{
		ActivateDisplay();
	}
}

int32 UMultiDisplayCameraComponent::GetNumDisplays()
{
	if (!FSlateApplication::IsInitialized())
	{
		return 1;
	}

	FDisplayMetrics DisplayMetrics;
	FSlateApplication::Get().GetDisplayMetrics(DisplayMetrics);

	return DisplayMetrics.MonitorInfo.Num();
}

bool UMultiDisplayCameraComponent::GetDisplayInfo(int32 DisplayIndex, FString& OutDisplayName, FIntPoint& OutResolution)
{
	if (!FSlateApplication::IsInitialized())
	{
		OutDisplayName = TEXT("Primary Display");
		OutResolution = FIntPoint(1920, 1080);
		return DisplayIndex == 0;
	}

	FDisplayMetrics DisplayMetrics;
	FSlateApplication::Get().GetDisplayMetrics(DisplayMetrics);

	if (DisplayIndex < 0 || DisplayIndex >= DisplayMetrics.MonitorInfo.Num())
	{
		return false;
	}

	const FMonitorInfo& Monitor = DisplayMetrics.MonitorInfo[DisplayIndex];
	OutDisplayName = FString::Printf(TEXT("Display %d: %s"), DisplayIndex, *Monitor.Name);
	OutResolution = FIntPoint(
		Monitor.NativeWidth > 0 ? Monitor.NativeWidth : Monitor.WorkArea.Right - Monitor.WorkArea.Left,
		Monitor.NativeHeight > 0 ? Monitor.NativeHeight : Monitor.WorkArea.Bottom - Monitor.WorkArea.Top
	);

	return true;
}

TArray<FString> UMultiDisplayCameraComponent::GetAllDisplayNames()
{
	TArray<FString> DisplayNames;

	if (!FSlateApplication::IsInitialized())
	{
		DisplayNames.Add(TEXT("Display 0: Primary"));
		return DisplayNames;
	}

	FDisplayMetrics DisplayMetrics;
	FSlateApplication::Get().GetDisplayMetrics(DisplayMetrics);

	for (int32 i = 0; i < DisplayMetrics.MonitorInfo.Num(); ++i)
	{
		const FMonitorInfo& Monitor = DisplayMetrics.MonitorInfo[i];
		FString DisplayName = FString::Printf(TEXT("Display %d: %s (%dx%d)"),
			i,
			*Monitor.Name,
			Monitor.NativeWidth > 0 ? Monitor.NativeWidth : Monitor.WorkArea.Right - Monitor.WorkArea.Left,
			Monitor.NativeHeight > 0 ? Monitor.NativeHeight : Monitor.WorkArea.Bottom - Monitor.WorkArea.Top
		);
		DisplayNames.Add(DisplayName);
	}

	return DisplayNames;
}

void UMultiDisplayCameraComponent::RefreshDisplayConfiguration()
{
	bool bWasActive = bIsDisplayActive;

	if (bIsDisplayActive)
	{
		DeactivateDisplay();
	}

	SetupRenderTarget();

	if (bWasActive)
	{
		ActivateDisplay();
	}
}

void UMultiDisplayCameraComponent::ApplyToDisplay()
{
	// Implementation handled by CreateSecondaryWindow
}

void UMultiDisplayCameraComponent::RemoveFromDisplay()
{
	DestroySecondaryWindow();
}

void UMultiDisplayCameraComponent::CreateSecondaryWindow()
{
	if (!FSlateApplication::IsInitialized())
	{
		UE_LOG(LogTemp, Warning, TEXT("MultiDisplayCamera: Slate not initialized, cannot create secondary window"));
		return;
	}

	// Destroy existing window first
	DestroySecondaryWindow();

	FDisplayMetrics DisplayMetrics;
	FSlateApplication::Get().GetDisplayMetrics(DisplayMetrics);

	if (TargetDisplayIndex >= DisplayMetrics.MonitorInfo.Num())
	{
		UE_LOG(LogTemp, Warning, TEXT("MultiDisplayCamera: Display %d not found, only %d displays available"),
			TargetDisplayIndex, DisplayMetrics.MonitorInfo.Num());
		return;
	}

	const FMonitorInfo& TargetMonitor = DisplayMetrics.MonitorInfo[TargetDisplayIndex];

	// Calculate window position and size
	int32 WindowX = TargetMonitor.WorkArea.Left;
	int32 WindowY = TargetMonitor.WorkArea.Top;
	int32 WindowWidth = TargetMonitor.WorkArea.Right - TargetMonitor.WorkArea.Left;
	int32 WindowHeight = TargetMonitor.WorkArea.Bottom - TargetMonitor.WorkArea.Top;

	// Use native resolution if available and fullscreen
	if (bFullscreen && TargetMonitor.NativeWidth > 0 && TargetMonitor.NativeHeight > 0)
	{
		WindowX = TargetMonitor.DisplayRect.Left;
		WindowY = TargetMonitor.DisplayRect.Top;
		WindowWidth = TargetMonitor.NativeWidth;
		WindowHeight = TargetMonitor.NativeHeight;
	}

	// Create window descriptor
	FText WindowTitle = FText::FromString(FString::Printf(TEXT("Camera View - Display %d"), TargetDisplayIndex));

	// Create the Slate window
	SecondaryWindow = SNew(SWindow)
		.Title(WindowTitle)
		.ClientSize(FVector2D(WindowWidth, WindowHeight))
		.ScreenPosition(FVector2D(WindowX, WindowY))
		.SizingRule(bFullscreen ? ESizingRule::FixedSize : ESizingRule::UserSized)
		.AutoCenter(EAutoCenter::None)
		.UseOSWindowBorder(!bFullscreen)
		.FocusWhenFirstShown(false)
		.SupportsMaximize(!bFullscreen)
		.SupportsMinimize(!bFullscreen)
		.HasCloseButton(!bFullscreen);

	// Create an image widget that displays the render target
	if (TextureTarget)
	{
		// Create a brush from the render target (stored as shared ptr for proper cleanup)
		RenderTargetBrush = MakeShared<FSlateBrush>();
		RenderTargetBrush->SetResourceObject(TextureTarget);
		RenderTargetBrush->ImageSize = FVector2D(WindowWidth, WindowHeight);
		RenderTargetBrush->DrawAs = ESlateBrushDrawType::Image;
		RenderTargetBrush->Tiling = ESlateBrushTileType::NoTile;

		DisplayWidget = SNew(SImage)
			.Image(RenderTargetBrush.Get());

		SecondaryWindow->SetContent(DisplayWidget.ToSharedRef());
	}

	// Add the window to Slate
	FSlateApplication::Get().AddWindow(SecondaryWindow.ToSharedRef(), true);

	// Make it fullscreen if requested
	if (bFullscreen)
	{
		SecondaryWindow->SetWindowMode(EWindowMode::WindowedFullscreen);
	}

	UE_LOG(LogTemp, Log, TEXT("MultiDisplayCamera: Created secondary window on display %d at (%d, %d) size %dx%d"),
		TargetDisplayIndex, WindowX, WindowY, WindowWidth, WindowHeight);
}

void UMultiDisplayCameraComponent::DestroySecondaryWindow()
{
	if (SecondaryWindow.IsValid())
	{
		if (FSlateApplication::IsInitialized())
		{
			FSlateApplication::Get().RequestDestroyWindow(SecondaryWindow.ToSharedRef());
		}
		SecondaryWindow.Reset();
		DisplayWidget.Reset();
		RenderTargetBrush.Reset();

		UE_LOG(LogTemp, Log, TEXT("MultiDisplayCamera: Destroyed secondary window"));
	}
}
