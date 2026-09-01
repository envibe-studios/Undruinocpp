// Mini CRT Weapon Display Component
// Reusable ActorComponent that pushes weapon ammo/state to the MiniCRT ESP32 via Andy.
//
// Whenever weapon ammo, magazine, reload state, or fire mode changes, this component
// builds and sends an ASCII serial command through the existing UAndySerialSubsystem
// connection (it never opens its own serial port):
//
//     !crt,{Side},{CurrentAmmo},{MaxAmmo},{FireMode},{Reloading}\n
//
// Andy receives the command and forwards it via ESP-NOW to the MiniCRT ESP32.
// This is Unreal-side command generation only - the Andy protocol is unchanged.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FiringComponent.h"
#include "MiniCRTWeaponDisplayComponent.generated.h"

// Forward declarations
class UGameInstanceSubsystem;

/** Dedicated log category for MiniCRT outgoing commands (used while testing). */
DECLARE_LOG_CATEGORY_EXTERN(LogMiniCRT, Log, All);

/** Which gun side this display represents. The numeric value is sent verbatim in the command. */
UENUM(BlueprintType)
enum class EMiniCRTSide : uint8
{
	Port      = 0 UMETA(DisplayName = "Port"),
	Starboard = 1 UMETA(DisplayName = "Starboard")
};

/**
 * Mini CRT Weapon Display Component
 *
 * Attach this component to a weapon actor (or the ship pawn that owns the gun).
 * Call SetMiniCRTState(...) whenever ammo / magazine / reload / fire mode changes,
 * or call UpdateMiniCRTDisplay() directly after mutating the cached values.
 *
 * The component:
 *  - Sends only on meaningful changes (BeginPlay, ammo change, mag change, reload
 *    start/complete, fire mode change). It never sends on Tick.
 *  - Clamps CurrentAmmo to [0, MaxAmmo] and MaxAmmo to >= 1.
 *  - Uppercases the fire mode text.
 *  - Sends an EMPTY frame when no magazine is loaded.
 *  - Rate-limits sends (default 50ms) and coalesces rapid updates into a single
 *    trailing send of the latest pending state.
 *
 * Setup (dual CRT):
 *  1. Add one component per gun (Port + Starboard when both CRTs are wired).
 *  2. Set Weapon Side on each instance (Port=0, Starboard=1).
 *  3. Set ShipId to match the port registered in UAndySerialSubsystem::AddPort.
 *  4. Drive each side from its weapon via SetCurrentAmmo / SetMaxAmmo / SetFireMode.
 *  Both sides use the same !crt command format; Andy routes by the Side field.
 */
UCLASS(ClassGroup=(Common), meta=(BlueprintSpawnableComponent), BlueprintType, Blueprintable)
class UNDUINOCPP_API UMiniCRTWeaponDisplayComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMiniCRTWeaponDisplayComponent();

	// === Configuration ===

	/** Ship identifier - must match the ShipId used in UAndySerialSubsystem::AddPort. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MiniCRT|Config")
	FName ShipId;

	/** Which CRT this component drives. Sent as the Side field in every command (0=Port, 1=Starboard).
	 *  Use one component instance per gun; both sides share the same !crt command format. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MiniCRT|Config", meta = (DisplayName = "Weapon Side"))
	EMiniCRTSide Side = EMiniCRTSide::Port;

	/** If true, only operate on the server (HasAuthority check). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MiniCRT|Config")
	bool bServerOnly = true;

	/** Minimum time between serial sends, in milliseconds. Updates faster than this are coalesced. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MiniCRT|Config", meta = (ClampMin = "0.0"))
	float DebounceMilliseconds = 50.0f;

	/** If true, send an initial frame on BeginPlay. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MiniCRT|Config")
	bool bSendOnBeginPlay = true;

	// === Cached weapon state (exposed for inspection / Blueprint reads) ===

	/** Current ammo in the active magazine. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MiniCRT|State")
	int32 CurrentAmmo = 0;

	/** Magazine capacity (clamped to >= 1 when sent). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MiniCRT|State")
	int32 MaxAmmo = 100;

	/** Fire mode text (e.g. BURST, AUTO, SCAN, TRACTOR, EMPTY). Uppercased when sent. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MiniCRT|State")
	FString FireMode = TEXT("EMPTY");

	/** Whether the weapon is currently reloading (sent as 0/1). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MiniCRT|State")
	bool bReloading = false;

	/** Whether a magazine is currently loaded. When false, an EMPTY frame is sent. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MiniCRT|State")
	bool bMagazineLoaded = false;

	// === Public API ===

	/**
	 * Update the current ammo and send the MiniCRT command.
	 * Marks a magazine as loaded. Only affects the wire ammo field for normal ammo modes;
	 * SCAN / TRACTOR / EMPTY always report CurrentAmmo = 0.
	 * @param InCurrentAmmo - Current ammo (clamped to [0, MaxAmmo]).
	 */
	UFUNCTION(BlueprintCallable, Category = "MiniCRT")
	void SetCurrentAmmo(int32 InCurrentAmmo);

	/**
	 * Update the magazine capacity and send the MiniCRT command.
	 * @param InMaxAmmo - Magazine capacity (clamped to >= 1). CurrentAmmo is re-clamped to fit.
	 */
	UFUNCTION(BlueprintCallable, Category = "MiniCRT")
	void SetMaxAmmo(int32 InMaxAmmo);

	/**
	 * Update the firing mode and send the MiniCRT command.
	 * Maps the firing mode to the wire text the MiniCRT expects:
	 *   Bullet -> BURST, Scanner -> SCAN, TractorBeam -> TRACTOR.
	 * @param InFireMode - The firing mode (matches the ship's EFiringModeType).
	 */
	UFUNCTION(BlueprintCallable, Category = "MiniCRT")
	void SetFireMode(EFiringModeType InFireMode);

	/**
	 * Set the fire mode from a raw text label (e.g. "BURST", "AUTO") and send.
	 * Use this for normal ammo magazines whose real weapon mode isn't one of the enum values.
	 * Text is uppercased on the wire.
	 * @param InFireMode - Fire mode label.
	 */
	UFUNCTION(BlueprintCallable, Category = "MiniCRT")
	void SetFireModeText(const FString& InFireMode);

	/** Scanner magazine: FireMode=SCAN, CurrentAmmo forced to 0, Reloading=0. Sends. */
	UFUNCTION(BlueprintCallable, Category = "MiniCRT")
	void SetScannerMagazine();

	/** Tractor beam magazine: FireMode=TRACTOR, CurrentAmmo forced to 0, Reloading=0. Sends. */
	UFUNCTION(BlueprintCallable, Category = "MiniCRT")
	void SetTractorMagazine();

	/** Convert a firing mode to the exact wire text the MiniCRT expects. */
	UFUNCTION(BlueprintPure, Category = "MiniCRT")
	static FString FiringModeToWire(EFiringModeType InFireMode);

	/** Returns the numeric side value sent in every CRT command (0=Port, 1=Starboard). */
	UFUNCTION(BlueprintPure, Category = "MiniCRT")
	int32 GetWeaponSideValue() const { return static_cast<int32>(Side); }

	/**
	 * Update the cached weapon state and send the MiniCRT command.
	 * Setting any value marks a magazine as loaded.
	 * @param InCurrentAmmo - Current ammo in the active magazine.
	 * @param InMaxAmmo - Magazine capacity.
	 * @param InFireMode - Fire mode text (will be uppercased).
	 * @param bInReloading - True if reloading.
	 */
	UFUNCTION(BlueprintCallable, Category = "MiniCRT")
	void SetMiniCRTState(int32 InCurrentAmmo, int32 InMaxAmmo, const FString& InFireMode, bool bInReloading);

	/**
	 * Build the command from the current cached state and send it (rate-limited).
	 * Call this after mutating the cached state directly, or to force a refresh.
	 */
	UFUNCTION(BlueprintCallable, Category = "MiniCRT")
	void UpdateMiniCRTDisplay();

	/**
	 * Mark whether a magazine is loaded. When set to false, the next send is an EMPTY frame.
	 * Sends immediately (rate-limited) to reflect the change.
	 * @param bLoaded - True if a magazine is loaded.
	 */
	UFUNCTION(BlueprintCallable, Category = "MiniCRT")
	void SetMagazineLoaded(bool bLoaded);

	/** No magazine / ejected: FireMode=EMPTY, CurrentAmmo=0, Reloading=1. Sends. */
	UFUNCTION(BlueprintCallable, Category = "MiniCRT")
	void ClearMagazine();

	/** Get the Andy serial subsystem (or nullptr if unavailable). */
	UFUNCTION(BlueprintPure, Category = "MiniCRT")
	UGameInstanceSubsystem* GetSerialSubsystem() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Build the exact command line (without trailing newline) from the current cached state. */
	FString BuildCommandLine() const;

	/** Immediately send the current cached state over serial and stamp the send time. */
	void SendNow();

	/** Timer callback that flushes the latest pending state after the debounce window. */
	void FlushPendingUpdate();

	/** Returns true if this component is allowed to operate (authority check). */
	bool ShouldOperate() const;

	/** Cached reference to the Andy serial subsystem (resolved at runtime). */
	UPROPERTY()
	TObjectPtr<UGameInstanceSubsystem> CachedSubsystem;

	/** Time of the last successful send, in seconds (FPlatformTime::Seconds). */
	double LastSendSeconds = -1.0;

	/** The exact command line last written to serial. Used to suppress redundant sends. */
	FString LastSentLine;

	/** True if an update arrived during the debounce window and is awaiting a trailing send. */
	bool bHasPendingUpdate = false;

	/** Timer used to flush the pending update once the debounce window elapses. */
	FTimerHandle DebounceTimerHandle;
};
