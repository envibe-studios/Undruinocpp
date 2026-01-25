// Arduino Communication Plugin - Ship Hardware Input Component Implementation

#include "ShipHardwareInputComponent.h"
#include "AndySerialSubsystem.h"
#include "EspPacketBP.h"
#include "FiringComponent.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Actor.h"

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
			UE_LOG(LogTemp, Log, TEXT("ShipHardwareInputComponent: Skipping bind on client (no authority) for ShipId '%s'"),
				*ShipId.ToString());
			return;
		}
	}

	// Validate ShipId is set
	if (ShipId.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("ShipHardwareInputComponent: ShipId not set on %s"),
			*GetOwner()->GetName());
		return;
	}

	BindToSubsystem();
}

void UShipHardwareInputComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
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
	// Filter by ShipId - only process events for our ship
	if (InShipId != ShipId)
	{
		return;
	}

	// Convert type byte to enum for switch
	EEspMsgType MsgType = UEspPacketBP::ByteToMsgType(Type);

	switch (MsgType)
	{
	case EEspMsgType::WeaponImu:
		{
			FWeaponImuData ImuData;
			if (UEspPacketBP::ParseWeaponImuPayload(Payload, ImuData))
			{
				bool bTriggerHeld = (ImuData.Buttons & 0x01) != 0;
				FQuat Orientation = ImuData.GetQuaternion();
				FVector EulerAngles = ImuData.EulerAngles;
				OnWeaponImu.Broadcast(Src, Type, Seq, Orientation, EulerAngles, bTriggerHeld, Payload);
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

				// Check if the Inserted state has changed and fire EvtTagChanged if so
				bool* PreviousState = WeaponTagInsertedState.Find(TagData.UID);
				if (!PreviousState || *PreviousState != TagData.bPresent)
				{
					WeaponTagInsertedState.Add(TagData.UID, TagData.bPresent);
					EvtTagChanged.Broadcast(TagData.UID, TagData.bPresent);

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
				OnReloadTag.Broadcast(Src, Type, Seq, TagData.UID, TagData.bPresent, Payload);
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
		if (Mag.TagId == TagId)
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
		WeaponMag.FiringMode,
		WeaponMag.Damage,
		WeaponMag.RateOfFire,
		WeaponMag.SpreadAngle,
		WeaponMag.BulletsPerShot,
		WeaponMag.MaxAmmo,
		WeaponMag.Range,
		WeaponMag.TractorPullForce,
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
