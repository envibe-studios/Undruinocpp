// Multi-Display Camera Component - Outputs camera view to selected display/monitor
//
// SETUP GUIDE:
// 1. Create an empty Actor in the level for EACH monitor you want to display on.
// 2. Add a MultiDisplayCameraComponent to each Actor.
// 3. Set "Target Display Index" to the desired monitor (0 = primary, 1 = second, etc.).
// 4. Position/rotate the Actor to point the camera where you want.
// 5. Optionally attach the component to a SpringArm or parent component for rotation inheritance.
// 6. Run as Standalone Game (not PIE) for best results.
// 7. A window will open on the target monitor showing the camera's view.
//
// IMPORTANT NOTES:
// - Each component creates a separate OS window on the target monitor.
// - "Display 0" = primary monitor, "Display 1" = second monitor, etc.
// - The component auto-activates on BeginPlay.
// - For best results, run as "Standalone Game" rather than PIE.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Styling/SlateBrush.h"
#include "MultiDisplayCameraComponent.generated.h"

class SWindow;
class SImage;

/**
 * Multi-Display Camera Component
 *
 * A SceneCaptureComponent2D that opens a secondary OS window on a chosen monitor
 * and displays the captured scene in real-time.
 *
 * Inherits all SceneCaptureComponent2D properties (ShowFlags, CaptureSource, etc.)
 * so you can configure capture quality, post-processing, and show/hide actors as usual.
 */
UCLASS(ClassGroup=(Camera), meta=(BlueprintSpawnableComponent))
class ARDUINOCOMMUNICATION_API UMultiDisplayCameraComponent : public USceneCaptureComponent2D
{
	GENERATED_BODY()

public:
	UMultiDisplayCameraComponent();

	// UActorComponent interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// === Display Settings ===

	/** The display/monitor index to output to (0 = primary display, 1 = second monitor, etc.) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MultiDisplay|Settings", meta = (ClampMin = "0", ClampMax = "7"))
	int32 TargetDisplayIndex = 1;

	/** Whether to use borderless fullscreen mode on the target display */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MultiDisplay|Settings")
	bool bFullscreen = true;

	/** Resolution width for the render target (0 = use display resolution) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MultiDisplay|Settings", meta = (ClampMin = "0", ClampMax = "7680"))
	int32 RenderTargetWidth = 0;

	/** Resolution height for the render target (0 = use display resolution) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MultiDisplay|Settings", meta = (ClampMin = "0", ClampMax = "4320"))
	int32 RenderTargetHeight = 0;

	/** Number of frames to wait after BeginPlay before opening the display window.
	 *  This gives the engine time to render into the render target so the window
	 *  doesn't start with blank content. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MultiDisplay|Settings", meta = (ClampMin = "1", ClampMax = "60"))
	int32 WindowOpenDelay = 8;

	// === Functions ===

	/** Activate this camera and open a window on the target display */
	UFUNCTION(BlueprintCallable, Category = "MultiDisplay")
	void ActivateDisplay();

	/** Deactivate this camera and close its display window */
	UFUNCTION(BlueprintCallable, Category = "MultiDisplay")
	void DeactivateDisplay();

	/** Check if this camera is currently active on a display */
	UFUNCTION(BlueprintPure, Category = "MultiDisplay")
	bool IsDisplayActive() const;

	/** Change the target display at runtime (will reopen the window on the new monitor) */
	UFUNCTION(BlueprintCallable, Category = "MultiDisplay")
	void SetTargetDisplay(int32 NewDisplayIndex);

	/** Get the current target display index */
	UFUNCTION(BlueprintPure, Category = "MultiDisplay")
	int32 GetTargetDisplay() const { return TargetDisplayIndex; }

	/** Get the number of available displays/monitors */
	UFUNCTION(BlueprintCallable, Category = "MultiDisplay")
	static int32 GetNumDisplays();

	/** Get display info for a specific monitor (name and resolution) */
	UFUNCTION(BlueprintCallable, Category = "MultiDisplay")
	static bool GetDisplayInfo(int32 DisplayIndex, FString& OutDisplayName, FIntPoint& OutResolution);

	/** Get an array of all available display names with resolutions */
	UFUNCTION(BlueprintCallable, Category = "MultiDisplay")
	static TArray<FString> GetAllDisplayNames();

	/** Get the render target being used for this camera */
	UFUNCTION(BlueprintPure, Category = "MultiDisplay")
	UTextureRenderTarget2D* GetRenderTarget() const { return TextureTarget; }

	/** Force refresh: closes and reopens the display window with current settings */
	UFUNCTION(BlueprintCallable, Category = "MultiDisplay")
	void RefreshDisplayConfiguration();

protected:
	/** Create or update the render target for this camera */
	void SetupRenderTarget();

private:
	/** Whether the display is currently active */
	bool bIsDisplayActive = false;

	/** Whether we are waiting to open the window (deferred open) */
	bool bPendingWindowOpen = false;

	/** Frame counter for deferred window opening */
	int32 FrameDelayCounter = 0;

	/** Cached display resolution */
	FIntPoint CachedDisplayResolution;

	/** Create a new secondary window on the target display */
	void CreateSecondaryWindow();

	/** Destroy the secondary window */
	void DestroySecondaryWindow();

	/** Update the window's image content from the render target */
	void UpdateWindowContent();

	/** Get a descriptive name for logging */
	FString GetDisplayLogPrefix() const;

	/** Handle to the secondary window */
	TSharedPtr<SWindow> SecondaryWindow;

	/** The Slate image widget displaying the render target */
	TSharedPtr<SImage> DisplayImage;

	/** Brush used to paint the render target into the SImage */
	FSlateBrush RenderTargetBrush;
};
