// DualJoystickTankInputComponent Implementation

#include "DualJoystickTankInputComponent.h"
#include "HoverMovementComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Misc/App.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <mmsystem.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogDualJoystickTank, Log, All);

UDualJoystickTankInputComponent::UDualJoystickTankInputComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UDualJoystickTankInputComponent::BeginPlay()
{
	Super::BeginPlay();

	ResolveMovementComponent();

	if (CachedMovement)
	{
		if (bAutoEnableStrafe)
		{
			CachedMovement->bEnableStrafe = true;

			// BP hovercrafts often leave MaxStrafeThrust at the C++ default (15k) while
			// MaxForwardThrust is tuned into the millions — strafe then feels broken.
			const float MinUsefulStrafe = CachedMovement->MaxForwardThrust * StrafeThrustScale;
			if (CachedMovement->MaxStrafeThrust < MinUsefulStrafe)
			{
				UE_LOG(LogDualJoystickTank, Log,
					TEXT("%s: Raising MaxStrafeThrust from %.0f to %.0f (%.0f%% of forward)."),
					*GetName(), CachedMovement->MaxStrafeThrust, MinUsefulStrafe, StrafeThrustScale * 100.0f);
				CachedMovement->MaxStrafeThrust = MinUsefulStrafe;
			}
		}

		// Apply stick input before hover forces each PrePhysics tick.
		CachedMovement->PrimaryComponentTick.AddPrerequisite(this, PrimaryComponentTick);
	}
	else
	{
		UE_LOG(LogDualJoystickTank, Error,
			TEXT("%s: No UHoverMovementComponent on %s — sticks will debug but not drive the craft."),
			*GetName(), GetOwner() ? *GetOwner()->GetName() : TEXT("(none)"));
	}

#if !PLATFORM_WINDOWS
	UE_LOG(LogDualJoystickTank, Warning,
		TEXT("DualJoystickTankInputComponent is Windows-only (winmm JoyN). Sticks will not be read on this platform."));
#endif
}

void UDualJoystickTankInputComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	HandleDebugToggle();

	if (!bEnabled)
	{
		return;
	}

	PollDevices();
	MixControls();
	ApplyToMovement();
	ApplyBoost();

	if (bDebugDisplay)
	{
		DrawDebug();
	}

	if (bDebugLog)
	{
		DebugLogAccumulator += DeltaTime;
		if (DebugLogAccumulator >= DebugLogInterval)
		{
			DebugLogAccumulator = 0.0f;
			UE_LOG(LogDualJoystickTank, Log,
				TEXT("Raw L(%.2f,%.2f)%s R(%.2f,%.2f)%s | Proc L(%.2f,%.2f) R(%.2f,%.2f) | F=%.2f Y=%.2f S=%.2f | Boost L=%d R=%d both=%d"),
				RawLeftX, RawLeftY, bLeftConnected ? TEXT("") : TEXT("[!]"),
				RawRightX, RawRightY, bRightConnected ? TEXT("") : TEXT("[!]"),
				LeftX, LeftY, RightX, RightY,
				ForwardInput, YawInput, StrafeInput,
				bLeftBoostButtonDown ? 1 : 0,
				bRightBoostButtonDown ? 1 : 0,
				(bLeftBoostButtonDown && bRightBoostButtonDown) ? 1 : 0);
		}
	}
}

void UDualJoystickTankInputComponent::SetDebugDisplay(bool bEnable)
{
	bDebugDisplay = bEnable;
}

void UDualJoystickTankInputComponent::ToggleDebugDisplay()
{
	bDebugDisplay = !bDebugDisplay;
}

void UDualJoystickTankInputComponent::ResolveMovementComponent()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	CachedMovement = Owner->FindComponentByClass<UHoverMovementComponent>();
	if (!CachedMovement)
	{
		UE_LOG(LogDualJoystickTank, Warning,
			TEXT("%s: No UHoverMovementComponent on owner %s"),
			*GetName(), *Owner->GetName());
	}
}

void UDualJoystickTankInputComponent::PollDevices()
{
	const int32 EffectiveLeftId = bSwapLeftRightDevices ? RightJoystickId : LeftJoystickId;
	const int32 EffectiveRightId = bSwapLeftRightDevices ? LeftJoystickId : RightJoystickId;

	ReadJoystickState(EffectiveLeftId, BoostButtonIndex, RawLeftX, RawLeftY, bLeftBoostButtonDown, LeftDeviceName, bLeftConnected);
	ReadJoystickState(EffectiveRightId, BoostButtonIndex, RawRightX, RawRightY, bRightBoostButtonDown, RightDeviceName, bRightConnected);

	LeftX = NormalizeAxis(RawLeftX, bInvertLeftX, LeftXDeadzone, ResponseExponent, MaxInputClamp);
	LeftY = NormalizeAxis(RawLeftY, bInvertLeftY, LeftYDeadzone, ResponseExponent, MaxInputClamp);
	RightX = NormalizeAxis(RawRightX, bInvertRightX, RightXDeadzone, ResponseExponent, MaxInputClamp);
	RightY = NormalizeAxis(RawRightY, bInvertRightY, RightYDeadzone, ResponseExponent, MaxInputClamp);
}

void UDualJoystickTankInputComponent::MixControls()
{
	// Tank mix:
	// Forward = average Y, Yaw = difference Y, Strafe = average X
	float Forward = 0.5f * (LeftY + RightY);
	float Yaw = 0.5f * (LeftY - RightY);
	float Strafe = 0.5f * (LeftX + RightX);

	Forward *= ForwardSensitivity;
	Yaw *= YawSensitivity;
	Strafe *= StrafeSensitivity;

	if (bInvertMixedYaw)
	{
		Yaw = -Yaw;
	}
	if (bInvertMixedStrafe)
	{
		Strafe = -Strafe;
	}

	ForwardInput = FMath::Clamp(Forward, -MaxInputClamp, MaxInputClamp);
	YawInput = FMath::Clamp(Yaw, -MaxInputClamp, MaxInputClamp);
	StrafeInput = FMath::Clamp(Strafe, -MaxInputClamp, MaxInputClamp);
}

void UDualJoystickTankInputComponent::ApplyToMovement()
{
	if (!bApplyToHoverMovement)
	{
		return;
	}

	if (!CachedMovement)
	{
		ResolveMovementComponent();
		if (!CachedMovement)
		{
			return;
		}
	}

	const float Activity =
		FMath::Abs(LeftX) + FMath::Abs(LeftY) + FMath::Abs(RightX) + FMath::Abs(RightY);

	const bool bActive = Activity > KINDA_SMALL_NUMBER;

	// Latch into HoverMovement so ESP/keyboard SetThrottle(0) cannot wipe sticks mid-frame.
	// Release the latch when idle so BPI_ESPComm / digital keys own the inputs again.
	if (!bActive && !bDrivingMovement)
	{
		return;
	}

	CachedMovement->SetExternalAnalogInput(ForwardInput, YawInput, StrafeInput, bActive);

	bDrivingMovement = bActive;
}

void UDualJoystickTankInputComponent::ApplyBoost()
{
	if (!bApplyBoostFromJoysticks || !bApplyToHoverMovement)
	{
		return;
	}

	if (!CachedMovement)
	{
		ResolveMovementComponent();
		if (!CachedMovement)
		{
			return;
		}
	}

	// Both button 3s must be held; releasing either cancels boost.
	const bool bBothHeld = bLeftBoostButtonDown && bRightBoostButtonDown;

	if (!bBothHeld && !bDrivingBoost)
	{
		return;
	}

	CachedMovement->SetBoostInput(bBothHeld);
	bDrivingBoost = bBothHeld;
}

void UDualJoystickTankInputComponent::HandleDebugToggle()
{
	if (!DebugToggleKey.IsValid())
	{
		return;
	}

	APlayerController* PC = nullptr;
	if (const UWorld* World = GetWorld())
	{
		PC = World->GetFirstPlayerController();
	}

	const bool bDown = PC && PC->IsInputKeyDown(DebugToggleKey);
	if (bDown && !bDebugToggleKeyWasDown)
	{
		ToggleDebugDisplay();
	}
	bDebugToggleKeyWasDown = bDown;
}

void UDualJoystickTankInputComponent::DrawDebug() const
{
	if (!GEngine)
	{
		return;
	}

	const FString Line1 = FString::Printf(
		TEXT("DualStick DEBUG (F8 toggle)  LeftId=%d%s  RightId=%d%s  Swap=%s"),
		bSwapLeftRightDevices ? RightJoystickId : LeftJoystickId,
		bLeftConnected ? TEXT(" OK") : TEXT(" MISSING"),
		bSwapLeftRightDevices ? LeftJoystickId : RightJoystickId,
		bRightConnected ? TEXT(" OK") : TEXT(" MISSING"),
		bSwapLeftRightDevices ? TEXT("YES") : TEXT("no"));

	const FString Line2 = FString::Printf(
		TEXT("Raw   L(X=%.2f Y=%.2f)  R(X=%.2f Y=%.2f)   [%s | %s]"),
		RawLeftX, RawLeftY, RawRightX, RawRightY,
		*LeftDeviceName, *RightDeviceName);

	const FString Line3 = FString::Printf(
		TEXT("Proc  L(X=%.2f Y=%.2f)  R(X=%.2f Y=%.2f)"),
		LeftX, LeftY, RightX, RightY);

	const FString Line4 = FString::Printf(
		TEXT("Mixed Forward=%.2f  Yaw=%.2f  Strafe=%.2f  Driving=%s"),
		ForwardInput, YawInput, StrafeInput,
		bDrivingMovement ? TEXT("yes") : TEXT("no"));

	const FString Line5 = CachedMovement
		? FString::Printf(TEXT("HoverMovement OK  throttle=%.2f steer=%.2f strafeIn=%.2f  strafeEn=%s  MaxStrafe=%.0f"),
			CachedMovement->GetThrottleInput(),
			CachedMovement->GetSteeringInput(),
			CachedMovement->GetCurrentStrafe(),
			CachedMovement->bEnableStrafe ? TEXT("yes") : TEXT("NO"),
			CachedMovement->MaxStrafeThrust)
		: FString(TEXT("HoverMovement MISSING — cannot drive craft"));

	const FString Line6 = FString::Printf(
		TEXT("Boost btn%d  L=%s R=%s  both=%s  drivingBoost=%s"),
		BoostButtonIndex,
		bLeftBoostButtonDown ? TEXT("DOWN") : TEXT("up"),
		bRightBoostButtonDown ? TEXT("DOWN") : TEXT("up"),
		(bLeftBoostButtonDown && bRightBoostButtonDown) ? TEXT("YES") : TEXT("no"),
		bDrivingBoost ? TEXT("yes") : TEXT("no"));

	GEngine->AddOnScreenDebugMessage(uint64(0xD5A11001), 0.0f, FColor::Cyan, Line1);
	GEngine->AddOnScreenDebugMessage(uint64(0xD5A11002), 0.0f, FColor::Yellow, Line2);
	GEngine->AddOnScreenDebugMessage(uint64(0xD5A11003), 0.0f, FColor::Green, Line3);
	GEngine->AddOnScreenDebugMessage(uint64(0xD5A11004), 0.0f, FColor::White, Line4);
	GEngine->AddOnScreenDebugMessage(uint64(0xD5A11005), 0.0f, CachedMovement ? FColor::Green : FColor::Red, Line5);
	GEngine->AddOnScreenDebugMessage(uint64(0xD5A11006), 0.0f,
		(bLeftBoostButtonDown && bRightBoostButtonDown) ? FColor::Orange : FColor::Silver, Line6);
}

bool UDualJoystickTankInputComponent::ReadJoystickState(int32 JoyId, int32 ButtonIndex1Based, float& OutX, float& OutY, bool& OutButtonDown, FString& OutName, bool& OutConnected)
{
	OutX = 0.0f;
	OutY = 0.0f;
	OutButtonDown = false;
	OutConnected = false;
	OutName = FString::Printf(TEXT("Joy%d"), JoyId);

#if PLATFORM_WINDOWS
	if (JoyId < 0 || JoyId > 15)
	{
		return false;
	}

	JOYCAPSW Caps;
	FMemory::Memzero(Caps);
	if (joyGetDevCapsW(static_cast<UINT>(JoyId), &Caps, sizeof(Caps)) != JOYERR_NOERROR)
	{
		OutName = FString::Printf(TEXT("Joy%d (absent)"), JoyId);
		return false;
	}

	OutName = FString::Printf(TEXT("%s (VID=%04X PID=%04X)"), Caps.szPname, Caps.wMid, Caps.wPid);

	JOYINFOEX Info;
	FMemory::Memzero(Info);
	Info.dwSize = sizeof(Info);
	Info.dwFlags = JOY_RETURNX | JOY_RETURNY | JOY_RETURNBUTTONS;

	if (joyGetPosEx(static_cast<UINT>(JoyId), &Info) != JOYERR_NOERROR)
	{
		return false;
	}

	OutConnected = true;

	const float XMin = static_cast<float>(Caps.wXmin);
	const float XMax = static_cast<float>(Caps.wXmax);
	const float YMin = static_cast<float>(Caps.wYmin);
	const float YMax = static_cast<float>(Caps.wYmax);

	const float XRange = FMath::Max(XMax - XMin, 1.0f);
	const float YRange = FMath::Max(YMax - YMin, 1.0f);

	// Map [min,max] -> [-1, +1]
	OutX = ((static_cast<float>(Info.dwXpos) - XMin) / XRange) * 2.0f - 1.0f;
	OutY = ((static_cast<float>(Info.dwYpos) - YMin) / YRange) * 2.0f - 1.0f;

	OutX = FMath::Clamp(OutX, -1.0f, 1.0f);
	OutY = FMath::Clamp(OutY, -1.0f, 1.0f);

	// JOYBUTTON1 = bit 0, JOYBUTTON3 = bit 2, etc.
	const int32 ClampedButton = FMath::Clamp(ButtonIndex1Based, 1, 32);
	const DWORD ButtonMask = 1u << (ClampedButton - 1);
	OutButtonDown = (Info.dwButtons & ButtonMask) != 0;

	return true;
#else
	return false;
#endif
}

float UDualJoystickTankInputComponent::NormalizeAxis(float RawMinus1To1, bool bInvert, float Deadzone, float Exponent, float ClampMax)
{
	float V = FMath::Clamp(RawMinus1To1, -1.0f, 1.0f);
	if (bInvert)
	{
		V = -V;
	}

	const float DZ = FMath::Clamp(Deadzone, 0.0f, 0.9f);
	const float AbsV = FMath::Abs(V);
	if (AbsV <= DZ)
	{
		return 0.0f;
	}

	// Rescale remaining range to 0..1 after deadzone
	float Rescaled = (AbsV - DZ) / (1.0f - DZ);
	Rescaled = FMath::Clamp(Rescaled, 0.0f, 1.0f);

	const float Exp = FMath::Max(Exponent, 0.25f);
	Rescaled = FMath::Pow(Rescaled, Exp);

	float Signed = (V >= 0.0f) ? Rescaled : -Rescaled;
	Signed = FMath::Clamp(Signed, -ClampMax, ClampMax);
	return Signed;
}
