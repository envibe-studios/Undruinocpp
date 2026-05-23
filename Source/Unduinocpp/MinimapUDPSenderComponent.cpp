#include "MinimapUDPSenderComponent.h"

#include "Common/UdpSocketBuilder.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "IPAddress.h"
#include "SocketSubsystem.h"
#include "Sockets.h"
#include "TimerManager.h"

UMinimapUDPSenderComponent::UMinimapUDPSenderComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UMinimapUDPSenderComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopSending();
	Super::EndPlay(EndPlayReason);
}

void UMinimapUDPSenderComponent::StartSending()
{
	if (!bIsSending)
	{
		if (!EnsureUdpSocket())
		{
			return;
		}
		bIsSending = true;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	World->GetTimerManager().ClearTimer(AutoSendTimerHandle);
	if (bAutoSend)
	{
		const float Interval = FMath::Max(SendInterval, 0.001f);
		World->GetTimerManager().SetTimer(
			AutoSendTimerHandle,
			this,
			&UMinimapUDPSenderComponent::TickAutoSend,
			Interval,
			true);
	}
}

void UMinimapUDPSenderComponent::StopSending()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AutoSendTimerHandle);
	}

	DestroyUdpSocket();
	bIsSending = false;
}

void UMinimapUDPSenderComponent::SetPiEndpoint(const FString& NewIPAddress, int32 NewPort)
{
	PiIPAddress = NewIPAddress;
	PiPort = FMath::Clamp(NewPort, 1, 65535);
}

void UMinimapUDPSenderComponent::SendMinimapState(float X, float Y, float Yaw)
{
	float OutX = 0.0f;
	float OutY = 0.0f;
	float OutYaw = 0.0f;
	TransformCoords(X, Y, Yaw, OutX, OutY, OutYaw);

	const FString Payload = FString::Printf(TEXT("%.2f,%.2f,%.2f"), OutX, OutY, OutYaw);
	SendPayload(Payload);
}

void UMinimapUDPSenderComponent::SendActorMinimapState(AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return;
	}

	const FVector Loc = Actor->GetActorLocation();
	const float Yaw = Actor->GetActorRotation().Yaw;
	SendMinimapState(Loc.X, Loc.Y, Yaw);
}

void UMinimapUDPSenderComponent::SendOwnerMinimapState()
{
	AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor))
	{
		return;
	}

	if (bUseOwnerTransform)
	{
		SendActorMinimapState(OwnerActor);
		return;
	}

	USceneComponent* Root = OwnerActor->GetRootComponent();
	if (!IsValid(Root))
	{
		SendActorMinimapState(OwnerActor);
		return;
	}

	const FVector Loc = Root->GetComponentLocation();
	const float Yaw = Root->GetComponentRotation().Yaw;
	SendMinimapState(Loc.X, Loc.Y, Yaw);
}

void UMinimapUDPSenderComponent::TickAutoSend()
{
	SendOwnerMinimapState();
}

bool UMinimapUDPSenderComponent::EnsureUdpSocket()
{
	if (UdpSocket)
	{
		return true;
	}

	ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	if (!SocketSubsystem)
	{
		return false;
	}

	UdpSocket = FUdpSocketBuilder(TEXT("MinimapUDPSender"))
		.AsReusable()
		.BoundToPort(0)
		.Build();

	return UdpSocket != nullptr;
}

void UMinimapUDPSenderComponent::DestroyUdpSocket()
{
	if (!UdpSocket)
	{
		return;
	}

	UdpSocket->Close();

	if (ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM))
	{
		SocketSubsystem->DestroySocket(UdpSocket);
	}

	UdpSocket = nullptr;
}

TSharedPtr<FInternetAddr> UMinimapUDPSenderComponent::ResolveRemoteAddress() const
{
	ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	if (!SocketSubsystem)
	{
		return nullptr;
	}

	TSharedRef<FInternetAddr> Addr = SocketSubsystem->CreateInternetAddr();

	bool bIsValid = false;
	Addr->SetIp(*PiIPAddress, bIsValid);
	if (!bIsValid)
	{
		return nullptr;
	}

	Addr->SetPort(PiPort);
	return Addr;
}

void UMinimapUDPSenderComponent::TransformCoords(float InX, float InY, float InYaw, float& OutX, float& OutY, float& OutYaw) const
{
	OutX = InX * CoordinateScale;
	OutY = InY * CoordinateScale;

	if (bInvertX)
	{
		OutX = -OutX;
	}
	if (bInvertY)
	{
		OutY = -OutY;
	}

	OutYaw = InYaw + YawOffset;
}

bool UMinimapUDPSenderComponent::SendPayload(const FString& Payload)
{
	if (!EnsureUdpSocket() || !UdpSocket)
	{
		return false;
	}

	const TSharedPtr<FInternetAddr> RemoteAddr = ResolveRemoteAddress();
	if (!RemoteAddr.IsValid())
	{
		return false;
	}

	const FTCHARToUTF8 Utf8(*Payload);
	const int32 Utf8Len = Utf8.Length();
	if (Utf8Len <= 0)
	{
		return false;
	}

	int32 BytesSent = 0;
	const bool bOk = UdpSocket->SendTo(
		reinterpret_cast<const uint8*>(Utf8.Get()),
		Utf8Len,
		BytesSent,
		*RemoteAddr);

	return bOk && BytesSent == Utf8Len;
}
