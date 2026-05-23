// Minimap UDP sender — sends "X,Y,Yaw" as UTF-8 over UDP for external minimap displays (e.g. Raspberry Pi).
//
// Blueprint usage:
// 1. Add "Minimap UDP Sender Component" to your player pawn (or any actor).
// 2. Set Pi IP Address (default 192.168.1.238) and Pi Port (default 9999) to match the Pi listener.
// 3. Optional: set Send Interval (seconds), Coordinate Scale, axis inversion, Yaw Offset.
// 4. To stream automatically: enable Auto Send, then call Start Sending from Begin Play (or when you want to start).
//    Or leave Auto Send off and call Send Owner Minimap State, Send Actor Minimap State, or Send Minimap State for one-off updates.
// 5. Call Stop Sending when done (socket is also closed on End Play).

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MinimapUDPSenderComponent.generated.h"

class FInternetAddr;
class FSocket;

/**
 * Sends plain-text minimap state over UDP: "X,Y,Yaw" (two decimal places), UTF-8 encoded.
 * Intended for a listener such as: x, y, yaw = map(float, data.decode().split(","))
 */
UCLASS(ClassGroup = (Networking), meta = (BlueprintSpawnableComponent), BlueprintType, Blueprintable)
class UNDUINOCPP_API UMinimapUDPSenderComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMinimapUDPSenderComponent();

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Target IPv4 address (or resolvable host) on the LAN. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap UDP")
	FString PiIPAddress = TEXT("192.168.1.238");

	/** UDP port the Pi listens on (e.g. 9999). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap UDP", meta = (ClampMin = "1", ClampMax = "65535"))
	int32 PiPort = 9999;

	/** Seconds between automatic sends when Auto Send is enabled. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap UDP", meta = (ClampMin = "0.001"))
	float SendInterval = 0.05f;

	/**
	 * When true, Start Sending will emit the owner transform on every Send Interval.
	 * When false, only explicit Send calls transmit (Start Sending still opens the socket if you use it).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap UDP")
	bool bAutoSend = false;

	/**
	 * If true, auto-send uses the owning actor's world location and yaw.
	 * If false, auto-send uses the root component's world location and yaw (can differ if root is rotated independently).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap UDP")
	bool bUseOwnerTransform = true;

	/** Multiplier applied to X and Y before send (Z is never sent). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap UDP")
	float CoordinateScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap UDP")
	bool bInvertX = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap UDP")
	bool bInvertY = false;

	/** Added to yaw (degrees) after reading the actor, before send. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap UDP")
	float YawOffset = 0.0f;

	/** Opens the UDP socket and, if Auto Send is enabled, begins periodic sends. */
	UFUNCTION(BlueprintCallable, Category = "Minimap UDP")
	void StartSending();

	/** Stops auto-send timer and closes the UDP socket. */
	UFUNCTION(BlueprintCallable, Category = "Minimap UDP")
	void StopSending();

	/**
	 * Send one packet. Inputs are Unreal world X/Y and yaw (degrees); scale, inversion, and Yaw Offset are applied here.
	 */
	UFUNCTION(BlueprintCallable, Category = "Minimap UDP")
	void SendMinimapState(float X, float Y, float Yaw);

	/** Send one packet using the actor's world X/Y and GetActorRotation().Yaw. */
	UFUNCTION(BlueprintCallable, Category = "Minimap UDP")
	void SendActorMinimapState(AActor* Actor);

	/**
	 * Send one packet for the owning actor's current X/Y/yaw.
	 * Uses the same source as auto-send: actor transform if Use Owner Transform is true, else root component world transform.
	 */
	UFUNCTION(BlueprintCallable, Category = "Minimap UDP")
	void SendOwnerMinimapState();

	/** Updates the remote endpoint used for all subsequent sends. */
	UFUNCTION(BlueprintCallable, Category = "Minimap UDP")
	void SetPiEndpoint(const FString& NewIPAddress, int32 NewPort);

	UFUNCTION(BlueprintPure, Category = "Minimap UDP")
	bool IsSending() const { return bIsSending; }

protected:
	void TickAutoSend();

private:
	bool EnsureUdpSocket();
	void DestroyUdpSocket();
	TSharedPtr<FInternetAddr> ResolveRemoteAddress() const;
	void TransformCoords(float InX, float InY, float InYaw, float& OutX, float& OutY, float& OutYaw) const;
	bool SendPayload(const FString& Payload);

	FSocket* UdpSocket = nullptr;
	FTimerHandle AutoSendTimerHandle;
	bool bIsSending = false;
};
