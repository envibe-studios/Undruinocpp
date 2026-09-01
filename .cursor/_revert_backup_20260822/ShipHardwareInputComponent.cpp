// Arduino Communication Plugin - Ship Hardware Input Component Implementation

#include "ShipHardwareInputComponent.h"
#include "AndySerialSubsystem.h"
#include "EspPacketBP.h"
#include "FiringComponent.h"
#include "WeaponImuLog.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "HAL/PlatformTime.h"
#include "TimerManager.h"
#include "UObject/UObjectIterator.h"
#include "UObject/UnrealType.h"

UShipHardwareInputComponent::UShipHardwareInputComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bWantsInitializeComponent = true;

	// Default to requiring server authority
	bServerOnly = true;
}

void UShipHardwareInputComponent::BeginPlay()
{
	Super::BeginPlay();

	// Skip binding if server-only mode and we don't have authority
	if (bServerOnly)
	{
		AActor* Owner = GetOwner();
		if (Owner && !Owner->HasAuthority())
		{
			UE_LOG(LogWeaponImu, Warning, TEXT("ShipHardwareInput[%s]: NOT bound — bServerOnly and no authority (client). IMU ignored on this instance."),
				*ShipId.ToString());
			return;
		}
	}

	// Validate ShipId is set
	if (ShipId.IsNone())
	{
		UE_LOG(LogWeaponImu, Error, TEXT("ShipHardwareInput[%s]: ShipId not set — will not receive IMU packets."),
			*GetOwner()->GetName());
		return;
	}

	if (!TryRegisterAsPrimaryHandler())
	{
		return;
	}

	ResolveFiringComponentRefs();
	BindToSubsystem();

	UE_LOG(LogWeaponImu, Warning, TEXT("ShipHardwareInput[%s] ShipId='%s' ACTIVE (autoApply=%s autoTriggerFire=%s forwardSendWeaponAim=%s forwardSendFire=%s) — filter Output Log: LogWeaponImu"),
		*GetOwner()->GetName(),
		*ShipId.ToString(),
		bAutoApplyImuRotation ? TEXT("on") : TEXT("off"),
		bAutoApplyTriggerFiring ? TEXT("on") : TEXT("off"),
		bForwardAimToSendWeaponAim ? TEXT("on") : TEXT("off"),
		bForwardTriggerToSendFire ? TEXT("on") : TEXT("off"));
}

void UShipHardwareInputComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopAimIsolationSweep();
	UnregisterPrimaryHandler();
	UnbindFromSubsystem();

	Super::EndPlay(EndPlayReason);
}

UAndySerialSubsystem* UShipHardwareInputComponent::GetSerialSubsystem() const
{
	if (CachedSubsystem)
	{
		return CachedSubsystem;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	UGameInstance* GameInstance = World->GetGameInstance();
	if (!GameInstance)
	{
		return nullptr;
	}

	return GameInstance->GetSubsystem<UAndySerialSubsystem>();
}

bool UShipHardwareInputComponent::IsConnected() const
{
	UAndySerialSubsystem* Subsystem = GetSerialSubsystem();
	if (!Subsystem)
	{
		return false;
	}

	return Subsystem->IsConnected(ShipId);
}

void UShipHardwareInputComponent::BindToSubsystem()
{
	if (bIsBound)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("ShipHardwareInputComponent: No World available for ShipId '%s'"),
			*ShipId.ToString());
		return;
	}

	UGameInstance* GameInstance = World->GetGameInstance();
	if (!GameInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("ShipHardwareInputComponent: No GameInstance available for ShipId '%s'"),
			*ShipId.ToString());
		return;
	}

	CachedSubsystem = GameInstance->GetSubsystem<UAndySerialSubsystem>();
	if (!CachedSubsystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("ShipHardwareInputComponent: UAndySerialSubsystem not available for ShipId '%s'"),
			*ShipId.ToString());
		return;
	}

	// Bind to subsystem events
	CachedSubsystem->OnFrameParsed.AddDynamic(this, &UShipHardwareInputComponent::OnFrameParsedHandler);
	CachedSubsystem->OnConnectionChanged.AddDynamic(this, &UShipHardwareInputComponent::OnConnectionChangedHandler);

	bIsBound = true;

	UE_LOG(LogTemp, Log, TEXT("ShipHardwareInputComponent: Bound to subsystem for ShipId '%s'"),
		*ShipId.ToString());
}

void UShipHardwareInputComponent::UnbindFromSubsystem()
{
	if (!bIsBound || !CachedSubsystem)
	{
		return;
	}

	CachedSubsystem->OnFrameParsed.RemoveDynamic(this, &UShipHardwareInputComponent::OnFrameParsedHandler);
	CachedSubsystem->OnConnectionChanged.RemoveDynamic(this, &UShipHardwareInputComponent::OnConnectionChangedHandler);

	bIsBound = false;
	CachedSubsystem = nullptr;

	UE_LOG(LogTemp, Log, TEXT("ShipHardwareInputComponent: Unbound from subsystem for ShipId '%s'"),
		*ShipId.ToString());
}

void UShipHardwareInputComponent::OnFrameParsedHandler(FName InShipId, uint8 Src, uint8 Type, int32 Seq, const TArray<uint8>& Payload)
{
	if (!bIsPrimaryHandler)
	{
		return;
	}

	if (Type == static_cast<uint8>(EEspMsgType::WeaponImu) && InShipId != ShipId)
	{
		UE_LOG(LogWeaponImu, Verbose, TEXT("WEAPON_IMU ignored: packet ShipId='%s' != component ShipId='%s' (src=%d)"),
			*InShipId.ToString(), *ShipId.ToString(), Src);
	}

	// Filter by ShipId - only process events for our ship
	if (InShipId != ShipId)
	{
		return;
	}

	if (Src == 3)
	{
		AnyCountPortSrc++;
	}
	else if (Src == 6)
	{
		AnyCountStarboardSrc++;
	}

	// Convert type byte to enum for switch
	EEspMsgType MsgType = UEspPacketBP::ByteToMsgType(Type);

	switch (MsgType)
	{
	case EEspMsgType::WeaponImu:
		{
			// PlayerController may not have a possessed pawn at BeginPlay — resolve gun refs lazily.
			if (!FiringComponentPort || !FiringComponentStarboard)
			{
				ResolveFiringComponentRefs();
			}

			FWeaponImuData ImuData;
			if (UEspPacketBP::ParseWeaponImuPayload(Payload, ImuData))
			{
				const uint8 ResolvedSide = ResolveImuSide(ImuData.Side, Src);
				bool bTriggerHeld = (ImuData.Buttons & 0x01) != 0;
				FQuat Orientation = ImuData.GetQuaternion();
				FVector EulerAngles = ImuData.EulerAngles;
				OnWeaponImu.Broadcast(Src, Type, Seq, Orientation, EulerAngles, bTriggerHeld, Payload);

				UFiringComponent* TargetFiring = ResolveFiringComponentForImu(ResolvedSide, Src);
				LogWeaponImuPacket(Src, Type, Seq, ImuData.Side, ResolvedSide, ImuData, TargetFiring);
				TrackWeaponImuStats(Src, ResolvedSide, TargetFiring);

				// Auto-apply IMU orientation to the side-matched FiringComponent.
				if (bAutoApplyImuRotation && TargetFiring)
				{
					TargetFiring->ApplyImuOrientation(Orientation);
				}

				// Visual gun meshes (Weapon_Left/Weapon_Right) are driven by BP SendWeaponAim on the hovercraft.
				if (bForwardAimToSendWeaponAim)
				{
					InvokeSendWeaponAimOnOwner(ResolvedSide, Orientation, bTriggerHeld);
				}

				if (bForwardTriggerToSendFire)
				{
					InvokeSendFireOnOwner(ResolvedSide, bTriggerHeld);
				}

				// Drive firing AFTER BP forwards so ProcessEvent/WeaponFire cannot clobber SetFiring.
				// SetFiring(true) internally ForceFireShot() so shots do not depend on Tick.
				if (bAutoApplyTriggerFiring && TargetFiring)
				{
					const bool bWasFiring = TargetFiring->IsFiring();
					if (bTriggerHeld)
					{
						static int32 HeldSetFiringLogCounter = 0;
						if ((++HeldSetFiringLogCounter % 30) == 1)
						{
							UE_LOG(LogWeaponImu, Warning, TEXT("WEAPON_IMU: applying SetFiring(true) on %s (alreadyFiring=%s mode path)"),
								*TargetFiring->GetName(),
								bWasFiring ? TEXT("yes") : TEXT("no"));
						}
					}
					TargetFiring->SetFiring(bTriggerHeld);
					if (bTriggerHeld != bWasFiring)
					{
						UE_LOG(LogWeaponImu, Warning, TEXT("WEAPON_IMU: SetFiring(%s) on %s (buttons=0x%02X side=%d)"),
							bTriggerHeld ? TEXT("true") : TEXT("false"),
							*TargetFiring->GetName(),
							ImuData.Buttons,
							ResolvedSide);
					}
				}
				else if (bTriggerHeld)
				{
					UE_LOG(LogWeaponImu, Error, TEXT("WEAPON_IMU: trigger held but cannot SetFiring (autoApplyTrigger=%s target=%s)"),
						bAutoApplyTriggerFiring ? TEXT("on") : TEXT("off"),
						TargetFiring ? *TargetFiring->GetName() : TEXT("<null>"));
				}
			}
			else
			{
				UE_LOG(LogWeaponImu, Warning, TEXT("WEAPON_IMU: parse failed src=%d type=%d seq=%d payloadLen=%d payload=%s"),
					Src, Type, Seq, Payload.Num(), *UEspPacketBP::BytesToHexString(Payload));
			}
		}
		break;

	case EEspMsgType::WheelTurn:
		{
			FWheelTurnData WheelData;
			if (UEspPacketBP::ParseWheelTurnPayload(Payload, WheelData))
			{
				// Convert direction bool to delta: right/clockwise = +1, left/counter-clockwise = -1
				int32 Delta = WheelData.bRight ? 1 : -1;
				OnWheelTurn.Broadcast(Src, Type, Seq, WheelData.WheelIndex, Delta, Payload);
			}
		}
		break;

	case EEspMsgType::JackState:
		{
			FJackStateData JackData;
			if (UEspPacketBP::ParseJackStatePayload(Payload, JackData))
			{
				OnJackState.Broadcast(Src, Type, Seq, JackData.State, Payload);
			}
		}
		break;

	case EEspMsgType::WeaponTag:
		{
			FWeaponTagData TagData;
			if (UEspPacketBP::ParseWeaponTagPayload(Payload, TagData))
			{
				OnWeaponTag.Broadcast(Src, Type, Seq, TagData.UID, TagData.bPresent, Payload);

				if (Src == 3)
				{
					TagCountPortSrc++;
				}
				else if (Src == 6)
				{
					TagCountStarboardSrc++;
				}

				UE_LOG(LogWeaponImu, Warning, TEXT("WEAPON_TAG: src=%d side=%d uid=%lld present=%s"),
					Src,
					TagData.Side,
					TagData.UID,
					TagData.bPresent ? TEXT("inserted") : TEXT("removed"));

				// Check if the Inserted state has changed and fire EvtTagChanged if so
				bool* PreviousState = WeaponTagInsertedState.Find(TagData.UID);
				if (!PreviousState || *PreviousState != TagData.bPresent)
				{
					WeaponTagInsertedState.Add(TagData.UID, TagData.bPresent);
					// ReaderIndex: 0=Port Weapon, 1=Starboard Weapon (from TagData.Side)
					EvtTagChanged.Broadcast(TagData.UID, TagData.bPresent, TagData.Side);

					// Auto-apply weapon mag configuration when tag is inserted
					if (bAutoApplyWeaponMag && TagData.bPresent)
					{
						ApplyWeaponMagByTagId(TagData.UID);
					}
				}
			}
		}
		break;

	case EEspMsgType::ReloadTag:
		{
			FReloadTagData TagData;
			if (UEspPacketBP::ParseReloadTagPayload(Payload, TagData))
			{
				// Firmware sometimes reports removal with UID=0. Attribute that to the last
				// occupied reload-bay tag so Blueprint gets a coherent remove transition.
				int64 EffectiveUID = TagData.UID;
				if (!TagData.bPresent && EffectiveUID == 0 && LastReloadBayUID != 0)
				{
					EffectiveUID = LastReloadBayUID;
				}

				OnReloadTag.Broadcast(Src, Type, Seq, EffectiveUID, TagData.bPresent, Payload);

				const bool bWasOccupied = bReloadBayOccupied;
				const int64 PreviousUID = LastReloadBayUID;
				const bool bOccupancyChanged = (TagData.bPresent != bWasOccupied)
					|| (TagData.bPresent && EffectiveUID != 0 && EffectiveUID != PreviousUID);

				if (bOccupancyChanged)
				{
					// If a different mag is inserted while one is already present, emit remove
					// for the previous UID first so consumers can cancel an in-flight reload.
					if (TagData.bPresent && bWasOccupied && PreviousUID != 0 && PreviousUID != EffectiveUID)
					{
						ReloadTagInsertedState.Add(PreviousUID, false);
						EvtTagChanged.Broadcast(PreviousUID, false, 2);
					}

					if (TagData.bPresent)
					{
						bReloadBayOccupied = true;
						LastReloadBayUID = EffectiveUID;
						ReloadTagInsertedState.Add(EffectiveUID, true);
						// ReaderIndex: 2=Reload Box. Do NOT auto-apply weapon mag here —
						// reload bay only tops off ammo; weapon bay (Type 4) owns equip.
						EvtTagChanged.Broadcast(EffectiveUID, true, 2);
					}
					else
					{
						bReloadBayOccupied = false;
						if (EffectiveUID != 0)
						{
							ReloadTagInsertedState.Add(EffectiveUID, false);
						}
						EvtTagChanged.Broadcast(EffectiveUID, false, 2);
						LastReloadBayUID = 0;
					}
				}
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("ShipHardwareInput[%s]: RELOAD_TAG (Type 5) dropped: LEN=%d (expected 5) payload=%s"),
					GetOwner() ? *GetOwner()->GetName() : TEXT("<none>"),
					Payload.Num(),
					*UEspPacketBP::BytesToHexString(Payload));
			}
		}
		break;

	default:
		// Unknown or unhandled message type - silently ignore
		break;
	}
}

void UShipHardwareInputComponent::OnConnectionChangedHandler(FName InShipId, bool bConnected)
{
	// Filter by ShipId - only process events for our ship
	if (InShipId != ShipId)
	{
		return;
	}

	if (!bConnected)
	{
		// Dropping the serial link clears reload-bay occupancy so a stale "present"
		// state cannot keep Blueprint reload loops alive after reconnect.
		if (bReloadBayOccupied)
		{
			const int64 ClearedUID = LastReloadBayUID;
			bReloadBayOccupied = false;
			LastReloadBayUID = 0;
			if (ClearedUID != 0)
			{
				ReloadTagInsertedState.Add(ClearedUID, false);
			}
			EvtTagChanged.Broadcast(ClearedUID, false, 2);
		}
	}

	OnShipConnectionChanged.Broadcast(bConnected);

	UE_LOG(LogTemp, Log, TEXT("ShipHardwareInputComponent: ShipId '%s' connection changed: %s"),
		*ShipId.ToString(), bConnected ? TEXT("Connected") : TEXT("Disconnected"));
}

// ============================================================================
// WEAPON MAG FUNCTIONS
// ============================================================================

bool UShipHardwareInputComponent::FindWeaponMagByTagId(int64 TagId, FWeaponMag& OutWeaponMag) const
{
	for (const FWeaponMag& Mag : WeaponMags)
	{
		if (Mag.TagId == TagId && Mag.bActive)
		{
			OutWeaponMag = Mag;
			return true;
		}
	}
	return false;
}

bool UShipHardwareInputComponent::ApplyWeaponMag(const FWeaponMag& WeaponMag)
{
	if (!FiringComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("ShipHardwareInputComponent: Cannot apply WeaponMag - FiringComponent not set"));
		return false;
	}

	FiringComponent->ApplyWeaponMagConfig(
		WeaponMag.bActive,
		static_cast<uint8>(WeaponMag.FiringMode),
		WeaponMag.Damage,
		WeaponMag.RateOfFire,
		WeaponMag.SpreadAngle,
		WeaponMag.BulletsPerShot,
		WeaponMag.MaxAmmo,
		WeaponMag.CurrentAmmo,
		WeaponMag.Range,
		WeaponMag.ScanDuration
	);

	UE_LOG(LogTemp, Log, TEXT("ShipHardwareInputComponent: Applied WeaponMag '%s' (TagId: %lld)"),
		*WeaponMag.WeaponName.ToString(), WeaponMag.TagId);

	return true;
}

bool UShipHardwareInputComponent::ApplyWeaponMagByTagId(int64 TagId)
{
	FWeaponMag FoundMag;
	if (FindWeaponMagByTagId(TagId, FoundMag))
	{
		return ApplyWeaponMag(FoundMag);
	}

	UE_LOG(LogTemp, Warning, TEXT("ShipHardwareInputComponent: No WeaponMag found for TagId: %lld"), TagId);
	return false;
}

namespace ShipHardwareInputPrivate
{
	static bool ComponentNameMatches(const UFiringComponent* Component, const TCHAR* Pattern)
	{
		return Component && Component->GetName().Contains(Pattern, ESearchCase::IgnoreCase, ESearchDir::FromStart);
	}

	static TMap<FName, TWeakObjectPtr<UShipHardwareInputComponent>> PrimaryHandlersByShipId;

	static int32 HandlerPriority(AActor* Owner)
	{
		if (!Owner)
		{
			return 0;
		}
		if (Owner->IsA<APlayerController>())
		{
			return 2;
		}
		if (Owner->IsA<APawn>())
		{
			return 1;
		}
		return 0;
	}
}

bool UShipHardwareInputComponent::TryRegisterAsPrimaryHandler()
{
	using namespace ShipHardwareInputPrivate;

	TWeakObjectPtr<UShipHardwareInputComponent>& Slot = PrimaryHandlersByShipId.FindOrAdd(ShipId);
	UShipHardwareInputComponent* Existing = Slot.Get();

	if (Existing && Existing != this)
	{
		const int32 ExistingPriority = HandlerPriority(Existing->GetOwner());
		const int32 ThisPriority = HandlerPriority(GetOwner());

		if (ThisPriority > ExistingPriority)
		{
			Existing->bIsPrimaryHandler = false;
			Existing->UnbindFromSubsystem();
			Existing->UnregisterPrimaryHandler();
			UE_LOG(LogWeaponImu, Warning, TEXT("ShipHardwareInput[%s]: taking over ShipId '%s' from %s"),
				*GetOwner()->GetName(), *ShipId.ToString(), *Existing->GetOwner()->GetName());
		}
		else
		{
			UE_LOG(LogWeaponImu, Warning, TEXT("ShipHardwareInput[%s]: duplicate ShipId '%s' — deferring to primary %s on %s"),
				*GetOwner()->GetName(), *ShipId.ToString(), *Existing->GetName(), *Existing->GetOwner()->GetName());
			return false;
		}
	}

	Slot = this;
	bIsPrimaryHandler = true;
	return true;
}

void UShipHardwareInputComponent::UnregisterPrimaryHandler()
{
	if (!bIsPrimaryHandler)
	{
		return;
	}

	using namespace ShipHardwareInputPrivate;
	if (TWeakObjectPtr<UShipHardwareInputComponent>* Slot = PrimaryHandlersByShipId.Find(ShipId))
	{
		if (Slot->Get() == this)
		{
			PrimaryHandlersByShipId.Remove(ShipId);
		}
	}

	bIsPrimaryHandler = false;
}

void UShipHardwareInputComponent::ResolveFiringComponentRefs()
{
	AActor* WeaponActor = ResolveWeaponActor();
	if (!WeaponActor)
	{
		return;
	}

	TArray<UFiringComponent*> FiringComponents;
	WeaponActor->GetComponents<UFiringComponent>(FiringComponents);

	for (UFiringComponent* Component : FiringComponents)
	{
		using namespace ShipHardwareInputPrivate;

		if (!FiringComponentPort
			&& (ComponentNameMatches(Component, TEXT("Left"))
				|| ComponentNameMatches(Component, TEXT("Port"))))
		{
			FiringComponentPort = Component;
		}

		if (!FiringComponentStarboard
			&& (ComponentNameMatches(Component, TEXT("Right"))
				|| ComponentNameMatches(Component, TEXT("Starboard"))
				|| ComponentNameMatches(Component, TEXT("Stbd"))))
		{
			FiringComponentStarboard = Component;
		}
	}

	if (!FiringComponent && FiringComponents.Num() == 1)
	{
		FiringComponent = FiringComponents[0];
	}

	UE_LOG(LogWeaponImu, Warning, TEXT("ShipHardwareInput[%s] weaponActor=%s: IMU routing -> Port=%s Starboard=%s Fallback=%s"),
		*GetOwner()->GetName(),
		*WeaponActor->GetName(),
		FiringComponentPort ? *FiringComponentPort->GetName() : TEXT("<unset>"),
		FiringComponentStarboard ? *FiringComponentStarboard->GetName() : TEXT("<unset>"),
		FiringComponent ? *FiringComponent->GetName() : TEXT("<unset>"));

	if (!bAutoApplyImuRotation && FiringComponents.Num() > 1)
	{
		UE_LOG(LogWeaponImu, Warning, TEXT("ShipHardwareInput[%s]: Dual guns but bAutoApplyImuRotation=false — enable it or use bForwardAimToSendWeaponAim."),
			*GetOwner()->GetName());
	}
}

AActor* UShipHardwareInputComponent::ResolveWeaponActor() const
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	if (APlayerController* PC = Cast<APlayerController>(Owner))
	{
		if (APawn* Pawn = PC->GetPawn())
		{
			return Pawn;
		}
	}

	return Owner;
}

uint8 UShipHardwareInputComponent::ResolveImuSide(uint8 PayloadSide, uint8 Src)
{
	// Known Andy device IDs: Port weapon = 3, Starboard weapon = 6.
	if (Src == 6 && PayloadSide != 1)
	{
		UE_LOG(LogWeaponImu, Warning, TEXT("WEAPON_IMU: Src=6 (Starboard device) but payload side=%d; using side 1"), PayloadSide);
		return 1;
	}

	if (Src == 3 && PayloadSide != 0)
	{
		UE_LOG(LogWeaponImu, Warning, TEXT("WEAPON_IMU: Src=3 (Port device) but payload side=%d; using side 0"), PayloadSide);
		return 0;
	}

	return PayloadSide;
}

UFiringComponent* UShipHardwareInputComponent::ResolveFiringComponentForImu(uint8 Side, uint8 Src) const
{
	if (Side == 0 && FiringComponentPort)
	{
		return FiringComponentPort;
	}

	if (Side == 1 && FiringComponentStarboard)
	{
		return FiringComponentStarboard;
	}

	// Src fallback when side-specific refs were not wired in the editor.
	if (Src == 3 && FiringComponentPort)
	{
		return FiringComponentPort;
	}

	if (Src == 6 && FiringComponentStarboard)
	{
		return FiringComponentStarboard;
	}

	// Legacy single-gun fallback: only use for Port/side 0 so Starboard is not silently dropped.
	if (Side == 0 && FiringComponent)
	{
		return FiringComponent;
	}

	return nullptr;
}

void UShipHardwareInputComponent::LogWeaponImuPacket(
	uint8 Src,
	uint8 Type,
	int32 Seq,
	uint8 PayloadSide,
	uint8 ResolvedSide,
	const FWeaponImuData& ImuData,
	UFiringComponent* TargetFiring) const
{
	const TCHAR* SideLabel = (ResolvedSide == 0) ? TEXT("PORT") : (ResolvedSide == 1) ? TEXT("STARBOARD") : TEXT("UNKNOWN");

	FString TargetLabel = TEXT("<none>");
	if (TargetFiring)
	{
		const AActor* TargetOwner = TargetFiring->GetOwner();
		TargetLabel = FString::Printf(TEXT("%s on %s"),
			*TargetFiring->GetName(),
			TargetOwner ? *TargetOwner->GetName() : TEXT("<no owner>"));
	}

	UE_LOG(LogWeaponImu, Warning, TEXT(
		"WEAPON_IMU: src=%d type=%d seq=%d payloadSide=%d resolvedSide=%d (%s) q=(%.4f,%.4f,%.4f,%.4f) buttons=0x%02X trigger=%s autoApply=%s sendWeaponAim=%s -> %s"),
		Src,
		Type,
		Seq,
		PayloadSide,
		ResolvedSide,
		SideLabel,
		ImuData.QuatX,
		ImuData.QuatY,
		ImuData.QuatZ,
		ImuData.QuatW,
		ImuData.Buttons,
		((ImuData.Buttons & 0x01) != 0) ? TEXT("held") : TEXT("released"),
		bAutoApplyImuRotation ? TEXT("on") : TEXT("off"),
		bForwardAimToSendWeaponAim ? TEXT("on") : TEXT("off"),
		*TargetLabel);
}

void UShipHardwareInputComponent::TrackWeaponImuStats(uint8 Src, uint8 ResolvedSide, UFiringComponent* TargetFiring)
{
	if (Src == 3)
	{
		ImuCountPortSrc++;
	}
	else if (Src == 6)
	{
		ImuCountStarboardSrc++;
	}
	else
	{
		ImuCountOtherSrc++;
	}

	if (!TargetFiring)
	{
		ImuCountNoTarget++;
	}

	const double Now = FPlatformTime::Seconds();
	if (LastImuStatsLogSeconds <= 0.0)
	{
		LastImuStatsLogSeconds = Now;
		return;
	}

	constexpr double StatsIntervalSeconds = 5.0;
	if ((Now - LastImuStatsLogSeconds) < StatsIntervalSeconds)
	{
		return;
	}

	const int32 Total = ImuCountPortSrc + ImuCountStarboardSrc + ImuCountOtherSrc;
	UE_LOG(LogWeaponImu, Warning, TEXT(
		"WEAPON_IMU stats (%.0fs): imu src3=%d src6=%d other=%d noTarget=%d total=%d | any src3=%d src6=%d | tag src3=%d src6=%d autoApply=%s sendWeaponAim=%s"),
		Now - LastImuStatsLogSeconds,
		ImuCountPortSrc,
		ImuCountStarboardSrc,
		ImuCountOtherSrc,
		ImuCountNoTarget,
		Total,
		AnyCountPortSrc,
		AnyCountStarboardSrc,
		TagCountPortSrc,
		TagCountStarboardSrc,
		bAutoApplyImuRotation ? TEXT("on") : TEXT("off"),
		bForwardAimToSendWeaponAim ? TEXT("on") : TEXT("off"));

	if (ImuCountPortSrc > 0 && ImuCountStarboardSrc == 0)
	{
		UE_LOG(LogWeaponImu, Warning, TEXT(
			"WEAPON_IMU stats: Port packets (src=3) arrived but ZERO Starboard (src=6) in last %.0fs — Andy likely not forwarding both IMU streams."),
			Now - LastImuStatsLogSeconds);
	}
	else if (ImuCountStarboardSrc > 0 && ImuCountPortSrc == 0)
	{
		UE_LOG(LogWeaponImu, Error, TEXT(
			"WEAPON_IMU stats: Starboard (src=6) arrived but ZERO Port IMU (src=3) in last %.0fs — Unreal is not receiving Port WeaponImu."),
			Now - LastImuStatsLogSeconds);
		if (AnyCountPortSrc == 0)
		{
			UE_LOG(LogWeaponImu, Error, TEXT(
				"WEAPON_IMU stats: ZERO packets of ANY type from src=3. Andy/ESP-NOW/Port ID path, not Port mesh."));
		}
		else if (TagCountPortSrc > 0)
		{
			UE_LOG(LogWeaponImu, Error, TEXT(
				"WEAPON_IMU stats: Port RFID is on the wire (tag src3=%d) but IMU is not — Port IMU sensor/wiring or firmware send path."),
				TagCountPortSrc);
		}
	}
	else if (ImuCountStarboardSrc > 0 && ImuCountNoTarget > 0)
	{
		UE_LOG(LogWeaponImu, Warning, TEXT(
			"WEAPON_IMU stats: Starboard packets arrived but %d had no FiringComponent target."),
			ImuCountNoTarget);
	}

	ImuCountPortSrc = 0;
	ImuCountStarboardSrc = 0;
	ImuCountOtherSrc = 0;
	ImuCountNoTarget = 0;
	AnyCountPortSrc = 0;
	AnyCountStarboardSrc = 0;
	TagCountPortSrc = 0;
	TagCountStarboardSrc = 0;
	LastImuStatsLogSeconds = Now;
}

void UShipHardwareInputComponent::InvokeSendWeaponAimOnOwner(uint8 ResolvedSide, const FQuat& Orientation, bool bTriggerHeld) const
{
	AActor* WeaponActor = ResolveWeaponActor();
	if (!WeaponActor)
	{
		return;
	}

	static const FName FunctionName(TEXT("SendWeaponAim"));
	UFunction* const Func = WeaponActor->FindFunction(FunctionName);
	if (!Func)
	{
		UE_LOG(LogWeaponImu, Warning, TEXT("SendWeaponAim not found on %s (ShipHardware owner=%s) — Weapon_Left/Weapon_Right will not move from C++."),
			*WeaponActor->GetName(),
			GetOwner() ? *GetOwner()->GetName() : TEXT("<none>"));
		return;
	}

	// Matches BPI_ESPComm::SendWeaponAim(bool Port, FQuat Quat, bool TriggerDown)
	struct FSendWeaponAimParams
	{
		bool Port = false;
		FQuat Quat = FQuat::Identity;
		bool TriggerDown = false;
	};

	FSendWeaponAimParams Params;
	Params.Port = (ResolvedSide == 0);
	Params.Quat = Orientation;
	Params.TriggerDown = bTriggerHeld;

	WeaponActor->ProcessEvent(Func, &Params);
}

void UShipHardwareInputComponent::InvokeSendFireOnOwner(uint8 ResolvedSide, bool bTriggerHeld) const
{
	AActor* WeaponActor = ResolveWeaponActor();
	if (!WeaponActor)
	{
		return;
	}

	static const FName FunctionName(TEXT("SendFire"));
	UFunction* const Func = WeaponActor->FindFunction(FunctionName);
	if (!Func)
	{
		UE_LOG(LogWeaponImu, Warning, TEXT("SendFire not found on %s (ShipHardware owner=%s) — trigger will not SetFiring."),
			*WeaponActor->GetName(),
			GetOwner() ? *GetOwner()->GetName() : TEXT("<none>"));
		return;
	}

	// Matches BPI_ESPComm::SendFire(bool TriggerDown, bool PortSide)
	struct FSendFireParams
	{
		bool TriggerDown = false;
		bool PortSide = false;
	};

	FSendFireParams Params;
	Params.TriggerDown = bTriggerHeld;
	Params.PortSide = (ResolvedSide == 0);

	WeaponActor->ProcessEvent(Func, &Params);
}

UShipHardwareInputComponent* UShipHardwareInputComponent::FindPrimaryInPlayWorld()
{
	UWorld* PlayWorld = nullptr;
	if (GEngine)
	{
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.World() && (Context.WorldType == EWorldType::PIE || Context.WorldType == EWorldType::Game))
			{
				PlayWorld = Context.World();
				break;
			}
		}
	}

	if (!PlayWorld)
	{
		UE_LOG(LogWeaponImu, Error, TEXT("WeaponImu isolation: no PIE/game world. Start play first."));
		return nullptr;
	}

	UShipHardwareInputComponent* Fallback = nullptr;
	for (TObjectIterator<UShipHardwareInputComponent> It; It; ++It)
	{
		UShipHardwareInputComponent* Comp = *It;
		if (!IsValid(Comp) || Comp->GetWorld() != PlayWorld)
		{
			continue;
		}
		if (Comp->bIsPrimaryHandler)
		{
			return Comp;
		}
		if (!Fallback)
		{
			Fallback = Comp;
		}
	}

	if (!Fallback)
	{
		UE_LOG(LogWeaponImu, Error, TEXT("WeaponImu isolation: no ShipHardwareInputComponent in play world."));
	}
	return Fallback;
}

void UShipHardwareInputComponent::ApplyIsolationAim(uint8 Side, const FQuat& Orientation) const
{
	UFiringComponent* TargetFiring = ResolveFiringComponentForImu(Side, Side == 0 ? 3 : 6);
	if (TargetFiring)
	{
		TargetFiring->ApplyImuOrientation(Orientation);
	}
	InvokeSendWeaponAimOnOwner(Side, Orientation, false);

	UE_LOG(LogWeaponImu, Warning, TEXT("WeaponImu isolation: applied %s aim q=(%.3f,%.3f,%.3f,%.3f) -> %s"),
		Side == 0 ? TEXT("PORT") : TEXT("STARBOARD"),
		Orientation.X, Orientation.Y, Orientation.Z, Orientation.W,
		TargetFiring ? *TargetFiring->GetName() : TEXT("<no FiringComponent>"));
}

void UShipHardwareInputComponent::ApplyFireIsolationBurst(uint8 Side)
{
	if (!FiringComponentPort || !FiringComponentStarboard)
	{
		ResolveFiringComponentRefs();
	}

	UFiringComponent* TargetFiring = ResolveFiringComponentForImu(Side, Side == 0 ? 3 : 6);
	if (!TargetFiring)
	{
		UE_LOG(LogWeaponImu, Error, TEXT("WeaponFire isolation: no FiringComponent for side %d"), Side);
		return;
	}

	TargetFiring->SetFiringMode(EFiringModeType::Bullet);
	TargetFiring->SetAmmo(30);
	TargetFiring->SetFiring(true);
	TargetFiring->ForceFireShot();
	UE_LOG(LogWeaponImu, Warning, TEXT("WeaponFire isolation BURST %s on %s — expect 'FireBullet ENTER' + orange debug line."),
		Side == 0 ? TEXT("PORT") : TEXT("STARBOARD"),
		*TargetFiring->GetName());
}

void UShipHardwareInputComponent::ApplyAimIsolationNudge(uint8 Side, float YawDegrees)
{
	if (!FiringComponentPort || !FiringComponentStarboard)
	{
		ResolveFiringComponentRefs();
	}

	const FQuat Orientation = FRotator(0.f, YawDegrees, 0.f).Quaternion();
	ApplyIsolationAim(Side, Orientation);
	UE_LOG(LogWeaponImu, Warning, TEXT(
		"WeaponImu isolation NUDGE %s yaw=%.0f — if this gun jumps, Unreal mesh/BP is fine."),
		Side == 0 ? TEXT("PORT / Weapon_Left") : TEXT("STARBOARD / Weapon_Right"),
		YawDegrees);
}

void UShipHardwareInputComponent::StopAimIsolationSweep()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AimSweepTimer);
	}
	AimSweepElapsed = 0.f;
}

void UShipHardwareInputComponent::TickAimIsolationSweep()
{
	AimSweepElapsed += 0.05f;
	const float Yaw = FMath::Sin(AimSweepElapsed * 2.2f) * 40.f;
	const float Pitch = FMath::Cos(AimSweepElapsed * 1.6f) * 18.f;
	ApplyIsolationAim(AimSweepSide, FRotator(Pitch, Yaw, 0.f).Quaternion());

	if (AimSweepElapsed >= AimSweepDuration)
	{
		StopAimIsolationSweep();
		UE_LOG(LogWeaponImu, Warning, TEXT("WeaponImu isolation SWEEP %s finished."),
			AimSweepSide == 0 ? TEXT("PORT") : TEXT("STARBOARD"));
	}
}

void UShipHardwareInputComponent::StartAimIsolationSweep(uint8 Side, float DurationSeconds)
{
	if (!FiringComponentPort || !FiringComponentStarboard)
	{
		ResolveFiringComponentRefs();
	}

	StopAimIsolationSweep();
	AimSweepSide = Side;
	AimSweepElapsed = 0.f;
	AimSweepDuration = FMath::Max(1.f, DurationSeconds);

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogWeaponImu, Error, TEXT("WeaponImu isolation: no world for sweep."));
		return;
	}

	World->GetTimerManager().SetTimer(
		AimSweepTimer,
		this,
		&UShipHardwareInputComponent::TickAimIsolationSweep,
		0.05f,
		true);

	UE_LOG(LogWeaponImu, Warning, TEXT(
		"WeaponImu isolation SWEEP %s for %.1fs — watch Weapon_%s. If it moves, Unreal is ruled out."),
		Side == 0 ? TEXT("PORT") : TEXT("STARBOARD"),
		AimSweepDuration,
		Side == 0 ? TEXT("Left") : TEXT("Right"));
}
