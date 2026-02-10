// Multi-Display Camera Component - Implementation

#include "MultiDisplayCameraComponent.h"
#include "Engine/Engine.h"
#include "Widgets/SWindow.h"
#include "Widgets/Images/SImage.h"
#include "Framework/Application/SlateApplication.h"

UMultiDisplayCameraComponent::UMultiDisplayCameraComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	// Auto-activate on BeginPlay by default (uses inherited bAutoActivate from UActorComponent)
	bAutoActivate = true;

	// Let the engine handle scene capture through the render pipeline.
	// This is far more efficient than manually calling CaptureScene() each tick,
	// as the engine batches and pipelines the GPU work properly.
	bCaptureEveryFrame = true;
	bCaptureOnMovement = true;
	bAlwaysPersistRenderingState = true;

	CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
}

void UMultiDisplayCameraComponent::BeginPlay()
{
	Super::BeginPlay();

	SetupRenderTarget();

	if (bAutoActivate)
	{
		ActivateDisplay();
	}
}

void UMultiDisplayCameraComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DeactivateDisplay();
	DestroySecondaryWindow();

	Super::EndPlay(EndPlayReason);
}

void UMultiDisplayCameraComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsDisplayActive)
	{
		return;
	}

	// The engine handles scene capture via bCaptureEveryFrame on the render thread.
	// We only need to invalidate the Slate widget so it repaints with the latest
	// render target content. Without this, Slate caches the widget and shows stale/gray.
	UpdateWindowContent();
}

void UMultiDisplayCameraComponent::SetupRenderTarget()
{
	int32 Width = RenderTargetWidth;
	int32 Height = RenderTargetHeight;

	// Auto-detect resolution from the target display
	if (Width == 0 || Height == 0)
	{
		FString DisplayName;
		FIntPoint DisplayRes;
		if (GetDisplayInfo(TargetDisplayIndex, DisplayName, DisplayRes))
		{
			Width = DisplayRes.X > 0 ? DisplayRes.X : 1920;
			Height = DisplayRes.Y > 0 ? DisplayRes.Y : 1080;
		}
		else
		{
			Width = 1920;
			Height = 1080;
		}
	}

	CachedDisplayResolution = FIntPoint(Width, Height);

	if (!TextureTarget)
	{
		TextureTarget = NewObject<UTextureRenderTarget2D>(this);
	}

	TextureTarget->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA8;
	TextureTarget->ClearColor = FLinearColor::Black;
	TextureTarget->bAutoGenerateMips = false;
	TextureTarget->InitAutoFormat(Width, Height);
	TextureTarget->UpdateResourceImmediate(true);

	// Ensure the GPU resource is fully created so Slate can sample from it
	TextureTarget->UpdateResource();

	UE_LOG(LogTemp, Log, TEXT("MultiDisplayCamera: Render target %dx%d for display %d"), Width, Height, TargetDisplayIndex);
}

void UMultiDisplayCameraComponent::ActivateDisplay()
{
	if (bIsDisplayActive)
	{
		return;
	}

	if (!TextureTarget)
	{
		SetupRenderTarget();
	}

	CreateSecondaryWindow();

	bIsDisplayActive = true;

	UE_LOG(LogTemp, Log, TEXT("MultiDisplayCamera: Activated on display %d"), TargetDisplayIndex);
}

void UMultiDisplayCameraComponent::DeactivateDisplay()
{
	if (!bIsDisplayActive)
	{
		return;
	}

	DestroySecondaryWindow();
	bIsDisplayActive = false;

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

	if (bIsDisplayActive)
	{
		DeactivateDisplay();
	}

	TargetDisplayIndex = FMath::Clamp(NewDisplayIndex, 0, 7);
	SetupRenderTarget();

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

void UMultiDisplayCameraComponent::CreateSecondaryWindow()
{
	if (!FSlateApplication::IsInitialized())
	{
		UE_LOG(LogTemp, Warning, TEXT("MultiDisplayCamera: Slate not initialized"));
		return;
	}

	// Clean up any existing window first
	DestroySecondaryWindow();

	FDisplayMetrics DisplayMetrics;
	FSlateApplication::Get().GetDisplayMetrics(DisplayMetrics);

	if (TargetDisplayIndex >= DisplayMetrics.MonitorInfo.Num())
	{
		UE_LOG(LogTemp, Warning, TEXT("MultiDisplayCamera: Display %d not found (%d displays available)"),
			TargetDisplayIndex, DisplayMetrics.MonitorInfo.Num());
		return;
	}

	const FMonitorInfo& TargetMonitor = DisplayMetrics.MonitorInfo[TargetDisplayIndex];

	// Determine window position and size from the target monitor
	int32 WindowX, WindowY, WindowWidth, WindowHeight;

	if (bFullscreen && TargetMonitor.NativeWidth > 0 && TargetMonitor.NativeHeight > 0)
	{
		// Use the full display rect for fullscreen
		WindowX = TargetMonitor.DisplayRect.Left;
		WindowY = TargetMonitor.DisplayRect.Top;
		WindowWidth = TargetMonitor.NativeWidth;
		WindowHeight = TargetMonitor.NativeHeight;
	}
	else
	{
		// Use the work area (excludes taskbar)
		WindowX = TargetMonitor.WorkArea.Left;
		WindowY = TargetMonitor.WorkArea.Top;
		WindowWidth = TargetMonitor.WorkArea.Right - TargetMonitor.WorkArea.Left;
		WindowHeight = TargetMonitor.WorkArea.Bottom - TargetMonitor.WorkArea.Top;
	}

	UE_LOG(LogTemp, Log, TEXT("MultiDisplayCamera: Opening window on display %d at (%d,%d) size %dx%d"),
		TargetDisplayIndex, WindowX, WindowY, WindowWidth, WindowHeight);

	// Setup the brush that will display the render target texture.
	// FSlateBrush::SetResourceObject points the brush at our UTextureRenderTarget2D.
	// Each frame we invalidate the SImage so Slate re-reads the texture data.
	RenderTargetBrush = FSlateBrush();
	RenderTargetBrush.DrawAs = ESlateBrushDrawType::Image;
	RenderTargetBrush.Tiling = ESlateBrushTileType::NoTile;
	RenderTargetBrush.ImageSize = FVector2D(WindowWidth, WindowHeight);
	RenderTargetBrush.ImageType = ESlateBrushImageType::FullColor;

	if (TextureTarget)
	{
		RenderTargetBrush.SetResourceObject(TextureTarget);
	}

	// Create the SImage widget. Using Image_Lambda ensures the brush pointer
	// is re-evaluated every frame (not cached once at construction).
	// The lambda also re-applies the resource object each frame to ensure
	// Slate's texture proxy stays synchronized with our render target.
	DisplayImage = SNew(SImage)
		.Image_Lambda([this]() -> const FSlateBrush*
		{
			if (TextureTarget)
			{
				RenderTargetBrush.SetResourceObject(TextureTarget);
			}
			return &RenderTargetBrush;
		});

	// Build the Slate window
	FText WindowTitle = FText::FromString(FString::Printf(TEXT("Camera - Display %d"), TargetDisplayIndex));

	SecondaryWindow = SNew(SWindow)
		.Title(WindowTitle)
		.ClientSize(FVector2D(WindowWidth, WindowHeight))
		.ScreenPosition(FVector2D(WindowX, WindowY))
		.AutoCenter(EAutoCenter::None)
		.SizingRule(bFullscreen ? ESizingRule::FixedSize : ESizingRule::UserSized)
		.UseOSWindowBorder(!bFullscreen)
		.FocusWhenFirstShown(false)
		.SupportsMaximize(!bFullscreen)
		.SupportsMinimize(!bFullscreen)
		.HasCloseButton(!bFullscreen)
		[
			DisplayImage.ToSharedRef()
		];

	FSlateApplication::Get().AddWindow(SecondaryWindow.ToSharedRef(), true);

	if (bFullscreen)
	{
		SecondaryWindow->SetWindowMode(EWindowMode::WindowedFullscreen);
	}
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
		DisplayImage.Reset();
		RenderTargetBrush.SetResourceObject(nullptr);

		UE_LOG(LogTemp, Log, TEXT("MultiDisplayCamera: Destroyed secondary window"));
	}
}

void UMultiDisplayCameraComponent::UpdateWindowContent()
{
	if (!DisplayImage.IsValid() || !SecondaryWindow.IsValid())
	{
		return;
	}

	// Ensure the brush still points at our render target (in case it was recreated)
	if (TextureTarget && RenderTargetBrush.GetResourceObject() != TextureTarget)
	{
		RenderTargetBrush.SetResourceObject(TextureTarget);
	}

	// Invalidate the image widget so Slate repaints with the latest render target content.
	// Only the SImage needs invalidation - the window will repaint automatically when
	// its child is dirty. Invalidating both caused unnecessary overhead.
	DisplayImage->Invalidate(EInvalidateWidgetReason::Paint);
}
