// Mini CRT Weapon Display Component Implementation

#include "MiniCRTWeaponDisplayComponent.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "UObject/Class.h"
#include "UObject/UObjectGlobals.h"

DEFINE_LOG_CATEGORY(LogMiniCRT);

namespace MiniCRTWeaponDisplay
{
	/** Resolve UAndySerialSubsystem at runtime to avoid a circular module dependency
	 *  (ArduinoCommunication already depends on Unduinocpp for FiringComponent). */
	UClass* GetAndySerialSubsystemClass()
	{
		static UClass* CachedClass = nullptr;
		if (!CachedClass)
		{
			CachedClass = FindObject<UClass>(nullptr, TEXT("/Script/ArduinoCommunication.AndySerialSubsystem"));
		}
		return CachedClass;
	}

	UGameInstanceSubsystem* ResolveAndySubsystem(UGameInstance* GameInstance)
	{
		if (!GameInstance)
		{
			return nullptr;
		}

		UClass* SubsystemClass = GetAndySerialSubsystemClass();
		if (!SubsystemClass)
		{
			return nullptr;
		}

		return GameInstance->GetSubsystemBase(SubsystemClass);
	}

	bool InvokeSendBytes(UGameInstanceSubsystem* Subsystem, FName ShipId, const TArray<uint8>& Data)
	{
		if (!Subsystem)
		{
			return false;
		}

		UFunction* SendBytesFunc = Subsystem->FindFunction(TEXT("SendBytes"));
		if (!SendBytesFunc)
		{
			return false;
		}

		struct FAndySendBytesParams
		{
			FName ShipId;
			TArray<uint8> Data;
			bool ReturnValue = false;
		};

		FAndySendBytesParams Params;
		Params.ShipId = ShipId;
		Params.Data = Data;

		Subsystem->ProcessEvent(SendBytesFunc, &Params);
		return Params.ReturnValue;
	}
}

UMiniCRTWeaponDisplayComponent::UMiniCRTWeaponDisplayComponent()
{
	// This component is event-driven; it must never send on Tick.
	PrimaryComponentTick.bCanEverTick = false;
	bWantsInitializeComponent = false;
}

void UMiniCRTWeaponDisplayComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!ShouldOperate())
	{
		return;
	}

	if (ShipId.IsNone())
	{
		UE_LOG(LogMiniCRT, Warning, TEXT("MiniCRT: ShipId not set on %s; commands will not be routed."),
			GetOwner() ? *GetOwner()->GetName() : TEXT("<unknown>"));
	}

	// Cache the subsystem up front so sends are cheap.
	CachedSubsystem = GetSerialSubsystem();

	if (bSendOnBeginPlay)
	{
		// Initialization frame (reflects whatever default/cached state is configured).
		UpdateMiniCRTDisplay();
	}
}

void UMiniCRTWeaponDisplayComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DebounceTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

bool UMiniCRTWeaponDisplayComponent::ShouldOperate() const
{
	if (bServerOnly)
	{
		const AActor* Owner = GetOwner();
		if (Owner && !Owner->HasAuthority())
		{
			return false;
		}
	}
	return true;
}

UGameInstanceSubsystem* UMiniCRTWeaponDisplayComponent::GetSerialSubsystem() const
{
	if (CachedSubsystem)
	{
		return CachedSubsystem;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	UGameInstance* GameInstance = World->GetGameInstance();
	if (!GameInstance)
	{
		return nullptr;
	}

	return MiniCRTWeaponDisplay::ResolveAndySubsystem(GameInstance);
}

FString UMiniCRTWeaponDisplayComponent::FiringModeToWire(EFiringModeType InFireMode)
{
	switch (InFireMode)
	{
	case EFiringModeType::Bullet:      return TEXT("BURST");
	case EFiringModeType::Scanner:     return TEXT("SCAN");
	case EFiringModeType::TractorBeam: return TEXT("TRACTOR");
	default:                           return TEXT("BURST");
	}
}

void UMiniCRTWeaponDisplayComponent::SetCurrentAmmo(int32 InCurrentAmmo)
{
	MaxAmmo = FMath::Max(1, MaxAmmo);
	CurrentAmmo = FMath::Clamp(InCurrentAmmo, 0, MaxAmmo);
	bMagazineLoaded = true;

	UpdateMiniCRTDisplay();
}

void UMiniCRTWeaponDisplayComponent::SetMaxAmmo(int32 InMaxAmmo)
{
	MaxAmmo = FMath::Max(1, InMaxAmmo);
	CurrentAmmo = FMath::Clamp(CurrentAmmo, 0, MaxAmmo);
	bMagazineLoaded = true;

	UpdateMiniCRTDisplay();
}

void UMiniCRTWeaponDisplayComponent::SetFireMode(EFiringModeType InFireMode)
{
	FireMode = FiringModeToWire(InFireMode);
	bMagazineLoaded = true;

	UpdateMiniCRTDisplay();
}

void UMiniCRTWeaponDisplayComponent::SetFireModeText(const FString& InFireMode)
{
	FireMode = InFireMode;
	bMagazineLoaded = true;

	UpdateMiniCRTDisplay();
}

void UMiniCRTWeaponDisplayComponent::SetScannerMagazine()
{
	FireMode = TEXT("SCAN");
	bMagazineLoaded = true;

	UpdateMiniCRTDisplay();
}

void UMiniCRTWeaponDisplayComponent::SetTractorMagazine()
{
	FireMode = TEXT("TRACTOR");
	bMagazineLoaded = true;

	UpdateMiniCRTDisplay();
}

void UMiniCRTWeaponDisplayComponent::SetMiniCRTState(int32 InCurrentAmmo, int32 InMaxAmmo, const FString& InFireMode, bool bInReloading)
{
	MaxAmmo = FMath::Max(1, InMaxAmmo);
	CurrentAmmo = FMath::Clamp(InCurrentAmmo, 0, MaxAmmo);
	FireMode = InFireMode;
	bReloading = bInReloading;
	bMagazineLoaded = true;

	UpdateMiniCRTDisplay();
}

void UMiniCRTWeaponDisplayComponent::SetMagazineLoaded(bool bLoaded)
{
	bMagazineLoaded = bLoaded;
	UpdateMiniCRTDisplay();
}

void UMiniCRTWeaponDisplayComponent::ClearMagazine()
{
	bMagazineLoaded = false;
	UpdateMiniCRTDisplay();
}

FString UMiniCRTWeaponDisplayComponent::BuildCommandLine() const
{
	const int32 SideValue = static_cast<int32>(Side);

	int32 OutMaxAmmo = FMath::Max(1, MaxAmmo);
	int32 OutCurrentAmmo = FMath::Clamp(CurrentAmmo, 0, OutMaxAmmo);

	// Resolve the fire mode text. No magazine loaded always reads as EMPTY.
	FString OutFireMode = bMagazineLoaded ? FireMode.ToUpper() : FString(TEXT("EMPTY"));
	if (OutFireMode.IsEmpty())
	{
		OutFireMode = TEXT("EMPTY");
	}

	// FireMode drives the special display states. The Arduino CRT sketch interprets
	// SCAN / TRACTOR / EMPTY visually; we only normalize the accompanying fields here.
	int32 OutReloading = bReloading ? 1 : 0;
	if (OutFireMode == TEXT("EMPTY"))
	{
		// No magazine inserted / magazine ejected: !crt,{Side},0,100,EMPTY,1
		OutCurrentAmmo = 0;
		OutMaxAmmo = 100;
		OutReloading = 1;
	}
	else if (OutFireMode == TEXT("SCAN") || OutFireMode == TEXT("TRACTOR"))
	{
		// Non-ammo special magazines: !crt,{Side},0,100,SCAN|TRACTOR,0
		OutCurrentAmmo = 0;
		OutMaxAmmo = 100;
		OutReloading = 0;
	}
	// else: normal ammo magazine -> send actual ammo, max, and the cached reloading flag.

	// Format: !crt,{Side},{CurrentAmmo},{MaxAmmo},{FireMode},{Reloading}
	return FString::Printf(TEXT("!crt,%d,%d,%d,%s,%d"),
		SideValue, OutCurrentAmmo, OutMaxAmmo, *OutFireMode, OutReloading);
}

void UMiniCRTWeaponDisplayComponent::UpdateMiniCRTDisplay()
{
	if (!ShouldOperate())
	{
		return;
	}

	// Only send when the resulting command actually changes (ammo, mag insert/remove,
	// reload start/end, fire mode change). Identical state is dropped so we never spam.
	const FString Candidate = BuildCommandLine();
	if (LastSendSeconds >= 0.0 && Candidate == LastSentLine)
	{
		return;
	}

	const double DebounceSeconds = FMath::Max(0.0f, DebounceMilliseconds) / 1000.0;
	const double Now = FPlatformTime::Seconds();

	// Send immediately if we're outside the debounce window (or there was no prior send).
	if (LastSendSeconds < 0.0 || DebounceSeconds <= 0.0 || (Now - LastSendSeconds) >= DebounceSeconds)
	{
		SendNow();
		return;
	}

	// Inside the debounce window: remember that the latest state needs to go out,
	// and schedule a single trailing send for when the window elapses.
	bHasPendingUpdate = true;

	if (UWorld* World = GetWorld())
	{
		if (!World->GetTimerManager().IsTimerActive(DebounceTimerHandle))
		{
			const float Remaining = static_cast<float>(DebounceSeconds - (Now - LastSendSeconds));
			World->GetTimerManager().SetTimer(
				DebounceTimerHandle, this,
				&UMiniCRTWeaponDisplayComponent::FlushPendingUpdate,
				FMath::Max(Remaining, 0.001f), false);
		}
	}
}

void UMiniCRTWeaponDisplayComponent::FlushPendingUpdate()
{
	if (bHasPendingUpdate)
	{
		SendNow();
	}
}

void UMiniCRTWeaponDisplayComponent::SendNow()
{
	bHasPendingUpdate = false;

	const FString Line = BuildCommandLine();

	// Defensive change-detection: if a coalesced/trailing send resolves to the same
	// command we already sent, drop it (no redundant serial writes).
	if (LastSendSeconds >= 0.0 && Line == LastSentLine)
	{
		return;
	}

	LastSendSeconds = FPlatformTime::Seconds();
	LastSentLine = Line;

	// Log the exact outgoing command (with the trailing \n shown explicitly) while testing.
	UE_LOG(LogMiniCRT, Log, TEXT("MiniCRT[%s] -> %s\\n"), *ShipId.ToString(), *Line);

	UGameInstanceSubsystem* Subsystem = GetSerialSubsystem();
	if (!Subsystem)
	{
		UE_LOG(LogMiniCRT, Warning, TEXT("MiniCRT: UAndySerialSubsystem unavailable; command not sent."));
		return;
	}

	// Send exact ASCII bytes including the trailing newline, independent of port LineEnding config.
	// This reuses the existing Andy serial connection registered under ShipId.
	const FString WireLine = Line + TEXT("\n");
	TArray<uint8> Bytes;
	Bytes.Reserve(WireLine.Len());
	for (TCHAR Ch : WireLine)
	{
		Bytes.Add(static_cast<uint8>(Ch));
	}

	if (!MiniCRTWeaponDisplay::InvokeSendBytes(Subsystem, ShipId, Bytes))
	{
		UE_LOG(LogMiniCRT, Warning, TEXT("MiniCRT: SendBytes failed for ShipId '%s' (port not open?)."),
			*ShipId.ToString());
	}
}
