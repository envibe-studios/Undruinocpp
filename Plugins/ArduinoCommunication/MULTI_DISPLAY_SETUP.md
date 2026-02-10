# Multi-Display Camera System - Setup Guide

Output any camera view to any connected monitor using the `UMultiDisplayCameraComponent`.

## Overview

The Multi-Display Camera Component extends `USceneCaptureComponent2D` to open a separate OS window on a chosen monitor, displaying the captured scene in real-time. Each component manages its own render target, Slate window, and per-frame updates automatically.

## Requirements

- Unreal Engine 5.7
- Two or more monitors connected to your system
- Monitors must be recognized by your OS (check display settings)

## Step-by-Step Setup

### Step 1: Create an Actor for Each Camera

1. In the Unreal Editor, create a new **Empty Actor** in your level (or use an existing Actor).
2. Position the Actor where you want the camera viewpoint to be.
3. If the camera should follow another object, attach the Actor to that object (or make it a child of another Actor).

### Step 2: Add the MultiDisplayCameraComponent

1. Select your Actor in the level.
2. In the **Details** panel, click **Add Component**.
3. Search for `MultiDisplayCameraComponent` and add it.
4. The component will appear under the Actor's component hierarchy.

### Step 3: Configure Display Settings

In the **Details** panel under the **MultiDisplay | Settings** category:

| Property | Default | Description |
|----------|---------|-------------|
| **Target Display Index** | `1` | Which monitor to output to. `0` = primary monitor, `1` = second monitor, `2` = third, etc. |
| **Fullscreen** | `true` | When enabled, the window opens in borderless fullscreen on the target monitor. |
| **Render Target Width** | `0` | Width in pixels. `0` = auto-detect from the monitor's native resolution. |
| **Render Target Height** | `0` | Height in pixels. `0` = auto-detect from the monitor's native resolution. |
| **Window Open Delay** | `4` | Number of frames to wait before opening the window. Gives the render target time to fill with valid content. Increase this if you see a brief flash of blank content on startup. |

### Step 4: Set Up Multiple Cameras (Multi-Monitor)

For a three-monitor setup, you need **three separate Actors**, each with its own `MultiDisplayCameraComponent`:

**Camera A (Primary Monitor):**
- Add `MultiDisplayCameraComponent` to Actor A
- Set `Target Display Index` = `0`
- Position Actor A where you want the primary view

**Camera B (Secondary Monitor):**
- Add `MultiDisplayCameraComponent` to Actor B
- Set `Target Display Index` = `1`
- Position Actor B where you want the secondary view

**Camera C (Third Monitor):**
- Add `MultiDisplayCameraComponent` to Actor C
- Set `Target Display Index` = `2`
- Position Actor C where you want the third view

> Each camera is an independent scene capture. They can look at different parts of the level, follow different actors, or even have different post-processing settings.

### Step 5: Run the Project

1. **Recommended**: Use **Standalone Game** mode (Play > Standalone Game) rather than Play-In-Editor (PIE). Standalone avoids Slate focus conflicts with the editor.
2. On launch, each component will:
   - Create a render target at the target monitor's resolution
   - Perform an initial scene capture
   - Wait a few frames (configurable via `WindowOpenDelay`) for the render target to have valid content
   - Open a Slate window on the target monitor
3. You should see each camera's view on the corresponding monitor.

## Camera Following / Rotation

The component inherits transform from its parent. To make the camera follow and rotate with an object:

**Option A: Attach to an Actor**
- Make the camera Actor a child of the object you want to follow (drag it onto the target Actor in the World Outliner).

**Option B: Use as a Component**
- Add the `MultiDisplayCameraComponent` directly to an existing Actor (e.g., a Pawn or Character). The component will move and rotate with its owning Actor.

**Option C: Spring Arm**
- Add a `SpringArmComponent` to your Actor first, then add the `MultiDisplayCameraComponent` as a child of the spring arm. This gives you camera lag, collision avoidance, etc.

The component has `bCaptureOnMovement = true` by default, so it automatically re-captures when the parent moves or rotates.

## Runtime Control (Blueprint / C++)

### Blueprint Functions

| Function | Description |
|----------|-------------|
| `ActivateDisplay()` | Open the window on the target monitor |
| `DeactivateDisplay()` | Close the window |
| `IsDisplayActive()` | Returns true if the window is open |
| `SetTargetDisplay(int32)` | Switch to a different monitor at runtime |
| `GetTargetDisplay()` | Get the current monitor index |
| `GetNumDisplays()` | Static: number of connected monitors |
| `GetDisplayInfo(int32, OutName, OutResolution)` | Static: get monitor name and resolution |
| `GetAllDisplayNames()` | Static: list all monitors with resolutions |
| `GetRenderTarget()` | Access the underlying `UTextureRenderTarget2D` |
| `RefreshDisplayConfiguration()` | Close and reopen the window with current settings |

### C++ Example

```cpp
// In your Actor header
UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
UMultiDisplayCameraComponent* DisplayCamera;

// In constructor
DisplayCamera = CreateDefaultSubobject<UMultiDisplayCameraComponent>(TEXT("DisplayCamera"));
DisplayCamera->TargetDisplayIndex = 1;
DisplayCamera->bFullscreen = true;
DisplayCamera->SetupAttachment(RootComponent);

// At runtime - switch monitors
DisplayCamera->SetTargetDisplay(2);

// Query available monitors
int32 NumDisplays = UMultiDisplayCameraComponent::GetNumDisplays();
TArray<FString> DisplayNames = UMultiDisplayCameraComponent::GetAllDisplayNames();
for (const FString& Name : DisplayNames)
{
    UE_LOG(LogTemp, Log, TEXT("Found: %s"), *Name);
}
```

### Blueprint Example

```
Event BeginPlay
    -> Get All Display Names
    -> For Each Loop
        -> Print String (display name)

Custom Event: SwitchMonitor (NewIndex)
    -> Set Target Display (NewIndex)
```

## SceneCaptureComponent2D Settings

Since `MultiDisplayCameraComponent` inherits from `USceneCaptureComponent2D`, you have access to all standard capture settings in the Details panel:

- **Capture Source**: `SCS_FinalColorLDR` (default), `SCS_SceneColorHDR`, `SCS_BaseColor`, etc.
- **Show Flags**: Control what renders (lighting, post-process, particles, etc.)
- **Hidden Actors / Show Only Actors**: Filter which actors appear in the capture
- **Post Process Settings**: Override post-processing per camera
- **FOV Angle**: Set field of view (under Projection settings)
- **Max View Distance Override**: Limit draw distance for performance

## Troubleshooting

### Blank/Gray Window on Secondary Monitor

1. **Check monitor detection**: In Blueprint, call `GetAllDisplayNames()` and print the result. Verify your target monitor appears in the list and note its index.
2. **Verify display index**: Ensure `Target Display Index` matches the monitor you want. Indices start at 0.
3. **Run as Standalone**: PIE mode can cause Slate window conflicts. Use Play > Standalone Game.
4. **Check for visible objects**: The camera captures what's in front of it. Ensure there are visible meshes/actors in the camera's view frustum.
5. **Increase Window Open Delay**: If the window opens before the render target has content, increase the `WindowOpenDelay` property (try 8 or 10).
6. **Check Output Log**: Look for lines starting with `MultiDisplayCamera:` for diagnostic messages.

### Performance Issues

- Each `MultiDisplayCameraComponent` performs an explicit `CaptureScene()` call each tick, which re-renders the scene from that camera's viewpoint. Multiple cameras multiply the rendering cost.
- **Lower resolution**: Set `RenderTargetWidth` and `RenderTargetHeight` to values below native (e.g., 1280x720 instead of 1920x1080).
- **Reduce capture scope**: Use Show Only Actors / Hidden Actors to limit what each camera renders.
- **Limit post-processing**: Disable expensive effects (bloom, SSR, SSAO) on secondary cameras via post-process overrides.
- **Reduce show flags**: Disable features like dynamic shadows or volumetric fog on secondary cameras.

### Window Appears on Wrong Monitor

- Monitor indices depend on OS display ordering, which may not match physical arrangement.
- Call `GetAllDisplayNames()` to see the exact index-to-monitor mapping.
- Use `SetTargetDisplay()` at runtime if the mapping differs from expected.

### Camera Not Following Parent Rotation

- Ensure the component is attached to its parent in the component hierarchy (not just at root level).
- The component sets `bCaptureOnMovement = true` by default, which re-captures when the parent transforms change.

## Architecture Notes

The component works by:

1. **Render Target**: Creates a `UTextureRenderTarget2D` sized to the target monitor's resolution.
2. **Explicit Scene Capture**: Calls `CaptureScene()` each tick to render the scene into the render target. This gives explicit control over when the render target has valid content, which is critical for multi-instance setups.
3. **Deferred Window Open**: Waits several frames (configurable) after activation before opening the Slate window, ensuring the render target has been written to by the GPU.
4. **Slate Window**: A separate `SWindow` is opened on the target monitor, containing an `SImage` widget.
5. **FSlateBrush**: Points at the render target texture via `SetResourceObject()`.
6. **Per-Frame Invalidation**: Each tick, the `SImage` is invalidated with `EInvalidateWidgetReason::Paint` so Slate resamples the render target texture instead of displaying a cached (stale) frame.
7. **Image_Lambda**: The `SImage` uses `Image_Lambda` for dynamic brush evaluation, ensuring the texture reference stays synchronized even if the render target is recreated.
