// Multi-Display Camera Component - Outputs camera view to selected display/monitor

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Styling/SlateBrush.h"
#include "MultiDisplayCameraComponent.generated.h"

/**
 * Multi-Display Camera Component
 * A camera component that allows selecting which display/monitor to output to.
 * Supports multiple monitors and provides Blueprint-friendly display selection.
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
	int32 TargetDisplayIndex = 0;

	/** Whether to automatically activate this camera on BeginPlay */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MultiDisplay|Settings")
	bool bAutoActivate = true;

	/** Whether to use fullscreen exclusive mode on the target display */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MultiDisplay|Settings")
	bool bFullscreen = true;

	/** Resolution width for the render target (0 = use display resolution) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MultiDisplay|Settings", meta = (ClampMin = "0", ClampMax = "7680"))
	int32 RenderTargetWidth = 0;

	/** Resolution height for the render target (0 = use display resolution) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MultiDisplay|Settings", meta = (ClampMin = "0", ClampMax = "4320"))
	int32 RenderTargetHeight = 0;

	/** Frame rate for the capture (0 = every frame) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MultiDisplay|Settings", meta = (ClampMin = "0", ClampMax = "240"))
	int32 CaptureFrameRate = 0;

	// === Functions ===

	/** Activate this camera and assign it to the target display */
	UFUNCTION(BlueprintCallable, Category = "MultiDisplay")
	void ActivateDisplay();

	/** Deactivate this camera from its display */
	UFUNCTION(BlueprintCallable, Category = "MultiDisplay")
	void DeactivateDisplay();

	/** Check if this camera is currently active on a display */
	UFUNCTION(BlueprintPure, Category = "MultiDisplay")
	bool IsDisplayActive() const;

	/** Change the target display at runtime */
	UFUNCTION(BlueprintCallable, Category = "MultiDisplay")
	void SetTargetDisplay(int32 NewDisplayIndex);

	/** Get the current target display index */
	UFUNCTION(BlueprintPure, Category = "MultiDisplay")
	int32 GetTargetDisplay() const { return TargetDisplayIndex; }

	/** Get the number of available displays/monitors */
	UFUNCTION(BlueprintCallable, Category = "MultiDisplay")
	static int32 GetNumDisplays();

	/** Get display info for a specific monitor */
	UFUNCTION(BlueprintCallable, Category = "MultiDisplay")
	static bool GetDisplayInfo(int32 DisplayIndex, FString& OutDisplayName, FIntPoint& OutResolution);

	/** Get an array of all available display names */
	UFUNCTION(BlueprintCallable, Category = "MultiDisplay")
	static TArray<FString> GetAllDisplayNames();

	/** Get the render target being used for this camera */
	UFUNCTION(BlueprintPure, Category = "MultiDisplay")
	UTextureRenderTarget2D* GetRenderTarget() const { return TextureTarget; }

	/** Force refresh the display configuration */
	UFUNCTION(BlueprintCallable, Category = "MultiDisplay")
	void RefreshDisplayConfiguration();

protected:
	/** Create or update the render target for this camera */
	void SetupRenderTarget();

	/** Apply the camera view to the target display */
	void ApplyToDisplay();

	/** Remove the camera view from the display */
	void RemoveFromDisplay();

private:
	/** Whether the display is currently active */
	bool bIsDisplayActive = false;

	/** Timer for frame rate limiting */
	float CaptureTimer = 0.0f;

	/** Cached display resolution */
	FIntPoint CachedDisplayResolution;

	/** Create a new secondary window for the display */
	void CreateSecondaryWindow();

	/** Destroy the secondary window */
	void DestroySecondaryWindow();

	/** Handle to the secondary window (if created) */
	TSharedPtr<class SWindow> SecondaryWindow;

	/** Widget to display the render target in the secondary window */
	TSharedPtr<class SWidget> DisplayWidget;

	/** Brush used to display the render target */
	TSharedPtr<FSlateBrush> RenderTargetBrush;
};
