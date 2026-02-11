// Multi-Display Camera Component - Implementation

#include "MultiDisplayCameraComponent.h"
#include "Engine/Engine.h"
#include "Widgets/SWindow.h"
#include "Widgets/Images/SImage.h"
#include "Framework/Application/SlateApplication.h"

DEFINE_LOG_CATEGORY_STATIC(LogMultiDisplay, Log, All);

UMultiDisplayCameraComponent::UMultiDisplayCameraComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	// Auto-activate on BeginPlay (inherited from UActorComponent)
	bAutoActivate = true;

	// Let the engine manage scene capture every frame.
	// This is the standard pipeline and properly handles multiple
	// SceneCaptureComponent2D instances rendering in the same frame.
	bCaptureEveryFrame = true;
	bCaptureOnMovement = true;
	bAlwaysPersistRenderingState = true;

	CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
}

FString UMultiDisplayCameraComponent::GetDisplayLogPrefix() const
{
	AActor* Owner = GetOwner();
	FString OwnerName = Owner ? Owner->GetName() : TEXT("NoOwner");
	return FString::Printf(TEXT("MultiDisplay[%s|Disp%d]"), *OwnerName, TargetDisplayIndex);
}

void UMultiDisplayCameraComponent::BeginPlay()
{
	// IMPORTANT: Set up the render target BEFORE calling Super::BeginPlay().
	// The engine's scene capture system registers this component during Super::BeginPlay()
	// and checks TextureTarget at that point. If TextureTarget is null when the engine
	// registers the component, the capture pipeline won't render into it, resulting in
	// blank output. By creating the render target first, the engine sees a valid target
	// during registration and properly sets up the GPU capture commands.
	SetupRenderTarget();

	Super::BeginPlay();

	UE_LOG(LogMultiDisplay, Log, TEXT("%s: BeginPlay (TextureTarget: %s, Resource: %s)"),
		*GetDisplayLogPrefix(),
		TextureTarget ? TEXT("valid") : TEXT("null"),
		(TextureTarget && TextureTarget->GetResource()) ? TEXT("ready") : TEXT("not ready"));

	// Do an initial capture to pre-fill the render target with content.
	// This ensures the render target has valid scene data before the deferred
	// window open, supplementing the engine's bCaptureEveryFrame pipeline.
	if (TextureTarget)
	{
		CaptureScene();
	}

	if (bAutoActivate)
	{
		ActivateDisplay();
	}
}

void UMultiDisplayCameraComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UE_LOG(LogMultiDisplay, Log, TEXT("%s: EndPlay"), *GetDisplayLogPrefix());

	DeactivateDisplay();
	DestroySecondaryWindow();

	Super::EndPlay(EndPlayReason);
}

void UMultiDisplayCameraComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsDisplayActive && !bPendingWindowOpen)
	{
		return;
	}

	// Deferred window open: wait N frames for the render target to have valid content
	if (bPendingWindowOpen)
	{
		FrameDelayCounter++;
		if (FrameDelayCounter >= WindowOpenDelay)
		{
			bPendingWindowOpen = false;
			CreateSecondaryWindow();
		}
		return;
	}

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

	// Each component instance must have its own unique render target.
	// Create a new one with a unique name to avoid any resource sharing issues.
	FName RTName = MakeUniqueObjectName(this, UTextureRenderTarget2D::StaticClass(), FName(FString::Printf(TEXT("MultiDisplayRT_%d"), TargetDisplayIndex)));
	TextureTarget = NewObject<UTextureRenderTarget2D>(this, RTName);

	TextureTarget->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA8;
	TextureTarget->ClearColor = FLinearColor::Black;
	TextureTarget->bAutoGenerateMips = false;
	TextureTarget->InitAutoFormat(Width, Height);
	TextureTarget->UpdateResourceImmediate(true);

	UE_LOG(LogMultiDisplay, Log, TEXT("%s: Render target created %dx%d (resource: %s)"),
		*GetDisplayLogPrefix(), Width, Height,
		TextureTarget->GetResource() ? TEXT("valid") : TEXT("null"));
}

void UMultiDisplayCameraComponent::ActivateDisplay()
{
	if (bIsDisplayActive)
	{
		return;
	}

	if (!TextureTarget)
	{
		UE_LOG(LogMultiDisplay, Warning, TEXT("%s: No render target, creating one"), *GetDisplayLogPrefix());
		SetupRenderTarget();
	}

	// Defer window creation to let the engine capture several frames first.
	// The engine's bCaptureEveryFrame=true will populate the render target
	// during these frames, ensuring the window shows content when it opens.
	bPendingWindowOpen = true;
	FrameDelayCounter = 0;
	bIsDisplayActive = true;

	UE_LOG(LogMultiDisplay, Log, TEXT("%s: Activated (window opens in %d frames)"), *GetDisplayLogPrefix(), WindowOpenDelay);
}

void UMultiDisplayCameraComponent::DeactivateDisplay()
{
	if (!bIsDisplayActive && !bPendingWindowOpen)
	{
		return;
	}

	bPendingWindowOpen = false;
	DestroySecondaryWindow();
	bIsDisplayActive = false;

	UE_LOG(LogMultiDisplay, Log, TEXT("%s: Deactivated"), *GetDisplayLogPrefix());
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
		UE_LOG(LogMultiDisplay, Warning, TEXT("%s: Slate not initialized"), *GetDisplayLogPrefix());
		return;
	}

	// Clean up any existing window first
	DestroySecondaryWindow();

	FDisplayMetrics DisplayMetrics;
	FSlateApplication::Get().GetDisplayMetrics(DisplayMetrics);

	if (TargetDisplayIndex >= DisplayMetrics.MonitorInfo.Num())
	{
		UE_LOG(LogMultiDisplay, Warning, TEXT("%s: Display %d not found (%d displays available)"),
			*GetDisplayLogPrefix(), TargetDisplayIndex, DisplayMetrics.MonitorInfo.Num());
		return;
	}

	const FMonitorInfo& TargetMonitor = DisplayMetrics.MonitorInfo[TargetDisplayIndex];

	// Determine window position and size from the target monitor
	int32 WindowX, WindowY, WindowWidth, WindowHeight;

	if (bFullscreen && TargetMonitor.NativeWidth > 0 && TargetMonitor.NativeHeight > 0)
	{
		WindowX = TargetMonitor.DisplayRect.Left;
		WindowY = TargetMonitor.DisplayRect.Top;
		WindowWidth = TargetMonitor.NativeWidth;
		WindowHeight = TargetMonitor.NativeHeight;
	}
	else
	{
		WindowX = TargetMonitor.WorkArea.Left;
		WindowY = TargetMonitor.WorkArea.Top;
		WindowWidth = TargetMonitor.WorkArea.Right - TargetMonitor.WorkArea.Left;
		WindowHeight = TargetMonitor.WorkArea.Bottom - TargetMonitor.WorkArea.Top;
	}

	UE_LOG(LogMultiDisplay, Log, TEXT("%s: Creating window at (%d,%d) size %dx%d"),
		*GetDisplayLogPrefix(), WindowX, WindowY, WindowWidth, WindowHeight);

	// Verify render target is ready
	if (!TextureTarget)
	{
		UE_LOG(LogMultiDisplay, Error, TEXT("%s: No render target available!"), *GetDisplayLogPrefix());
		return;
	}

	if (!TextureTarget->GetResource())
	{
		UE_LOG(LogMultiDisplay, Warning, TEXT("%s: Render target resource not yet available, forcing update"), *GetDisplayLogPrefix());
		TextureTarget->UpdateResourceImmediate(true);
	}

	// Setup the brush that will display the render target texture.
	// We store a raw pointer to the render target that the lambda will use.
	// This avoids issues with brush resource object caching.
	RenderTargetBrush = FSlateBrush();
	RenderTargetBrush.DrawAs = ESlateBrushDrawType::Image;
	RenderTargetBrush.Tiling = ESlateBrushTileType::NoTile;
	RenderTargetBrush.ImageSize = FVector2D(WindowWidth, WindowHeight);
	RenderTargetBrush.ImageType = ESlateBrushImageType::FullColor;
	RenderTargetBrush.SetResourceObject(TextureTarget);

	UE_LOG(LogMultiDisplay, Log, TEXT("%s: Brush resource set to %s (RenderTarget: %s, Resource: %s)"),
		*GetDisplayLogPrefix(),
		*TextureTarget->GetName(),
		TextureTarget->IsValidLowLevel() ? TEXT("valid") : TEXT("invalid"),
		TextureTarget->GetResource() ? TEXT("ready") : TEXT("null"));

	// Capture a raw pointer to the render target for the lambda.
	// The lambda needs to re-set the resource object every frame to ensure
	// Slate picks up the latest GPU content from the texture.
	UTextureRenderTarget2D* RT = TextureTarget;
	FSlateBrush* BrushPtr = &RenderTargetBrush;

	// Create the SImage widget. Using Image_Lambda ensures the brush pointer
	// is re-evaluated every frame (not cached once at construction).
	DisplayImage = SNew(SImage)
		.Image_Lambda([BrushPtr, RT]() -> const FSlateBrush*
		{
			if (RT && RT->IsValidLowLevel() && RT->GetResource())
			{
				BrushPtr->SetResourceObject(RT);
			}
			return BrushPtr;
		});

	// Mark the image widget as Volatile so Slate never caches its paint data.
	// Without this, Slate's invalidation system may still serve cached (blank)
	// frames even when Invalidate() is called, because the GPU texture update
	// doesn't trigger Slate's change detection. ForceVolatile ensures the
	// widget geometry and paint are recalculated every single frame.
	DisplayImage->ForceVolatile(true);

	// Build the Slate window with a title that identifies this camera
	AActor* Owner = GetOwner();
	FString OwnerName = Owner ? Owner->GetActorNameOrLabel() : TEXT("Camera");
	FText WindowTitle = FText::FromString(FString::Printf(TEXT("Camera - Display %d (%s)"), TargetDisplayIndex, *OwnerName));

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

	UE_LOG(LogMultiDisplay, Log, TEXT("%s: Window opened successfully"), *GetDisplayLogPrefix());
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

		UE_LOG(LogMultiDisplay, Log, TEXT("%s: Window destroyed"), *GetDisplayLogPrefix());
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
		UE_LOG(LogMultiDisplay, Log, TEXT("%s: Brush resource re-linked to render target"), *GetDisplayLogPrefix());
	}

	// Invalidate the image widget so Slate repaints with the latest render target content.
	// Without this, Slate caches the widget and shows a stale/blank frame.
	DisplayImage->Invalidate(EInvalidateWidgetReason::Paint);
}
