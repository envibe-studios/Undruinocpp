// Hover Movement Component Implementation

#include "HoverMovementComponent.h"
#include "HoverThrusterComponent.h"
#include "GameFramework/Actor.h"
#include "Components/PrimitiveComponent.h"
#include "DrawDebugHelpers.h"

UHoverMovementComponent::UHoverMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UHoverMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	// Cache physics component
	CachedPhysicsComponent = GetPhysicsComponent();

	// Auto-register thrusters from owning actor
	AutoRegisterThrusters();
}

void UHoverMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bMovementEnabled)
	{
		return;
	}

	// Update digital input to raw input
	if (bForwardPressed && !bBackwardPressed)
	{
		RawThrottleInput = 1.0f;
	}
	else if (bBackwardPressed && !bForwardPressed)
	{
		RawThrottleInput = -1.0f;
	}
	else if (!bForwardPressed && !bBackwardPressed)
	{
		// Only reset if no analog override was set this frame
		// This allows analog input to work alongside digital
	}

	if (bLeftPressed && !bRightPressed)
	{
		RawSteeringInput = -1.0f;
	}
	else if (bRightPressed && !bLeftPressed)
	{
		RawSteeringInput = 1.0f;
	}
	else if (!bLeftPressed && !bRightPressed)
	{
		// Only reset if no analog override
	}

	if (bEnableStrafe)
	{
		if (bStrafeLeftPressed && !bStrafeRightPressed)
		{
			RawStrafeInput = -1.0f;
		}
		else if (bStrafeRightPressed && !bStrafeLeftPressed)
		{
			RawStrafeInput = 1.0f;
		}
	}

	// Update smoothed input values
	UpdateInputSmoothing(DeltaTime);

	// Apply forces
	ApplyThrust(DeltaTime);
	ApplyTurning(DeltaTime);

	if (bEnableStrafe)
	{
		ApplyStrafeForce(DeltaTime);
	}

	ApplyDrag(DeltaTime);
}

// ============================================================================
// INPUT FUNCTIONS - Analog
// ============================================================================

void UHoverMovementComponent::SetThrottleInput(float Value)
{
	float OldValue = RawThrottleInput;
	RawThrottleInput = FMath::Clamp(Value, -1.0f, 1.0f);

	if (OldValue != RawThrottleInput)
	{
		OnThrottleChanged.Broadcast(RawThrottleInput);
		OnMovementInputChanged.Broadcast(RawThrottleInput, RawSteeringInput);
	}
}

void UHoverMovementComponent::SetSteeringInput(float Value)
{
	float OldValue = RawSteeringInput;
	RawSteeringInput = FMath::Clamp(Value, -1.0f, 1.0f);

	if (OldValue != RawSteeringInput)
	{
		OnSteeringChanged.Broadcast(RawSteeringInput);
		OnMovementInputChanged.Broadcast(RawThrottleInput, RawSteeringInput);
	}
}

void UHoverMovementComponent::SetStrafeInput(float Value)
{
	if (bEnableStrafe)
	{
		RawStrafeInput = FMath::Clamp(Value, -1.0f, 1.0f);
	}
}

// ============================================================================
// INPUT FUNCTIONS - Digital
// ============================================================================

void UHoverMovementComponent::MoveForward(bool bPressed)
{
	bForwardPressed = bPressed;
	if (!bPressed && !bBackwardPressed)
	{
		// When releasing, clear raw input so smoothing takes over
		RawThrottleInput = 0.0f;
	}
}

void UHoverMovementComponent::MoveBackward(bool bPressed)
{
	bBackwardPressed = bPressed;
	if (!bPressed && !bForwardPressed)
	{
		RawThrottleInput = 0.0f;
	}
}

void UHoverMovementComponent::TurnLeft(bool bPressed)
{
	bLeftPressed = bPressed;
	if (!bPressed && !bRightPressed)
	{
		RawSteeringInput = 0.0f;
	}
}

void UHoverMovementComponent::TurnRight(bool bPressed)
{
	bRightPressed = bPressed;
	if (!bPressed && !bLeftPressed)
	{
		RawSteeringInput = 0.0f;
	}
}

void UHoverMovementComponent::StrafeLeft(bool bPressed)
{
	bStrafeLeftPressed = bPressed;
	if (!bPressed && !bStrafeRightPressed)
	{
		RawStrafeInput = 0.0f;
	}
}

void UHoverMovementComponent::StrafeRight(bool bPressed)
{
	bStrafeRightPressed = bPressed;
	if (!bPressed && !bStrafeLeftPressed)
	{
		RawStrafeInput = 0.0f;
	}
}

// ============================================================================
// STATE QUERY FUNCTIONS
// ============================================================================

float UHoverMovementComponent::GetCurrentThrottle() const
{
	return CurrentThrottle;
}

float UHoverMovementComponent::GetCurrentSteering() const
{
	return CurrentSteering;
}

float UHoverMovementComponent::GetCurrentStrafe() const
{
	return CurrentStrafe;
}

float UHoverMovementComponent::GetThrottleInput() const
{
	return RawThrottleInput;
}

float UHoverMovementComponent::GetSteeringInput() const
{
	return RawSteeringInput;
}

float UHoverMovementComponent::GetForwardSpeed() const
{
	UPrimitiveComponent* PhysComp = GetPhysicsComponent();
	if (!PhysComp)
	{
		return 0.0f;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return 0.0f;
	}

	FVector Velocity = PhysComp->GetPhysicsLinearVelocity();
	FVector ForwardVector = Owner->GetActorForwardVector();

	return FVector::DotProduct(Velocity, ForwardVector);
}

float UHoverMovementComponent::GetLateralSpeed() const
{
	UPrimitiveComponent* PhysComp = GetPhysicsComponent();
	if (!PhysComp)
	{
		return 0.0f;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return 0.0f;
	}

	FVector Velocity = PhysComp->GetPhysicsLinearVelocity();
	FVector RightVector = Owner->GetActorRightVector();

	return FVector::DotProduct(Velocity, RightVector);
}

float UHoverMovementComponent::GetSpeed() const
{
	UPrimitiveComponent* PhysComp = GetPhysicsComponent();
	if (!PhysComp)
	{
		return 0.0f;
	}

	return PhysComp->GetPhysicsLinearVelocity().Size();
}

bool UHoverMovementComponent::IsGrounded() const
{
	for (UHoverThrusterComponent* Thruster : RegisteredThrusters)
	{
		if (Thruster && Thruster->IsGroundDetected())
		{
			return true;
		}
	}
	return false;
}

// ============================================================================
// CONTROL FUNCTIONS
// ============================================================================

void UHoverMovementComponent::SetMovementEnabled(bool bEnabled)
{
	bMovementEnabled = bEnabled;
	if (!bEnabled)
	{
		ResetInput();
	}
}

bool UHoverMovementComponent::IsMovementEnabled() const
{
	return bMovementEnabled;
}

void UHoverMovementComponent::ResetInput()
{
	RawThrottleInput = 0.0f;
	RawSteeringInput = 0.0f;
	RawStrafeInput = 0.0f;
	CurrentThrottle = 0.0f;
	CurrentSteering = 0.0f;
	CurrentStrafe = 0.0f;

	bForwardPressed = false;
	bBackwardPressed = false;
	bLeftPressed = false;
	bRightPressed = false;
	bStrafeLeftPressed = false;
	bStrafeRightPressed = false;
}

void UHoverMovementComponent::RegisterThruster(UHoverThrusterComponent* Thruster)
{
	if (Thruster && !RegisteredThrusters.Contains(Thruster))
	{
		RegisteredThrusters.Add(Thruster);
	}
}

void UHoverMovementComponent::UnregisterThruster(UHoverThrusterComponent* Thruster)
{
	RegisteredThrusters.Remove(Thruster);
}

// ============================================================================
// PROTECTED FUNCTIONS
// ============================================================================

void UHoverMovementComponent::AutoRegisterThrusters()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	TArray<UHoverThrusterComponent*> FoundThrusters;
	Owner->GetComponents<UHoverThrusterComponent>(FoundThrusters);

	for (UHoverThrusterComponent* Thruster : FoundThrusters)
	{
		RegisterThruster(Thruster);
	}
}

void UHoverMovementComponent::UpdateInputSmoothing(float DeltaTime)
{
	if (bSmoothInput)
	{
		// Smooth throttle
		float ThrottleDiff = RawThrottleInput - CurrentThrottle;
		if (FMath::Abs(ThrottleDiff) > KINDA_SMALL_NUMBER)
		{
			float SmoothSpeed = (FMath::Abs(RawThrottleInput) > FMath::Abs(CurrentThrottle))
				? ThrustAcceleration : ThrustDeceleration;
			CurrentThrottle = FMath::FInterpTo(CurrentThrottle, RawThrottleInput, DeltaTime, SmoothSpeed * ThrottleSmoothingSpeed);
		}
		else
		{
			CurrentThrottle = RawThrottleInput;
		}

		// Smooth steering
		float SteeringDiff = RawSteeringInput - CurrentSteering;
		if (FMath::Abs(SteeringDiff) > KINDA_SMALL_NUMBER)
		{
			float SmoothSpeed = (FMath::Abs(RawSteeringInput) > FMath::Abs(CurrentSteering))
				? SteeringAcceleration : SteeringDeceleration;
			CurrentSteering = FMath::FInterpTo(CurrentSteering, RawSteeringInput, DeltaTime, SmoothSpeed * SteeringSmoothingSpeed);
		}
		else
		{
			CurrentSteering = RawSteeringInput;
		}

		// Smooth strafe
		CurrentStrafe = FMath::FInterpTo(CurrentStrafe, RawStrafeInput, DeltaTime, ThrottleSmoothingSpeed);
	}
	else
	{
		CurrentThrottle = RawThrottleInput;
		CurrentSteering = RawSteeringInput;
		CurrentStrafe = RawStrafeInput;
	}
}

void UHoverMovementComponent::ApplyThrust(float DeltaTime)
{
	if (FMath::IsNearlyZero(CurrentThrottle))
	{
		return;
	}

	UPrimitiveComponent* PhysComp = GetPhysicsComponent();
	if (!PhysComp || !PhysComp->IsSimulatingPhysics())
	{
		return;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	// Calculate thrust force
	float ThrustMagnitude = (CurrentThrottle > 0.0f)
		? CurrentThrottle * MaxForwardThrust
		: CurrentThrottle * MaxBackwardThrust; // Note: CurrentThrottle is negative here

	// Get forward direction
	FVector ForwardVector = Owner->GetActorForwardVector();
	FVector ThrustForce = ForwardVector * ThrustMagnitude;

	// Calculate application point
	FVector ApplicationPoint = Owner->GetActorLocation();
	ApplicationPoint.Z += ThrustHeightOffset;

	// Apply force
	PhysComp->AddForceAtLocation(ThrustForce, ApplicationPoint);

	// Debug visualization
	if (bDrawDebug)
	{
		DrawDebugDirectionalArrow(
			GetWorld(),
			ApplicationPoint,
			ApplicationPoint + ThrustForce.GetSafeNormal() * 200.0f,
			20.0f,
			CurrentThrottle > 0.0f ? FColor::Green : FColor::Red,
			false,
			-1.0f,
			0,
			3.0f
		);
	}
}

void UHoverMovementComponent::ApplyTurning(float DeltaTime)
{
	if (FMath::IsNearlyZero(CurrentSteering))
	{
		return;
	}

	UPrimitiveComponent* PhysComp = GetPhysicsComponent();
	if (!PhysComp || !PhysComp->IsSimulatingPhysics())
	{
		return;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	// Calculate turn multiplier based on speed if enabled
	float TurnMultiplier = GetSpeedBasedTurnMultiplier();

	// Calculate torque
	float TorqueMagnitude = CurrentSteering * MaxTurnTorque * TurnMultiplier;

	// Apply torque around up axis (yaw)
	FVector UpVector = Owner->GetActorUpVector();
	FVector Torque = UpVector * TorqueMagnitude;

	PhysComp->AddTorqueInRadians(Torque);

	// Debug visualization
	if (bDrawDebug)
	{
		FVector DebugStart = Owner->GetActorLocation() + FVector(0, 0, 100);
		DrawDebugDirectionalArrow(
			GetWorld(),
			DebugStart,
			DebugStart + (CurrentSteering > 0.0f ? Owner->GetActorRightVector() : -Owner->GetActorRightVector()) * 100.0f,
			20.0f,
			FColor::Blue,
			false,
			-1.0f,
			0,
			3.0f
		);
	}
}

void UHoverMovementComponent::ApplyStrafeForce(float DeltaTime)
{
	if (!bEnableStrafe || FMath::IsNearlyZero(CurrentStrafe))
	{
		return;
	}

	UPrimitiveComponent* PhysComp = GetPhysicsComponent();
	if (!PhysComp || !PhysComp->IsSimulatingPhysics())
	{
		return;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	// Calculate strafe force
	float StrafeMagnitude = CurrentStrafe * MaxStrafeThrust;
	FVector RightVector = Owner->GetActorRightVector();
	FVector StrafeForce = RightVector * StrafeMagnitude;

	// Apply force at center
	PhysComp->AddForce(StrafeForce);

	// Debug visualization
	if (bDrawDebug)
	{
		FVector DebugStart = Owner->GetActorLocation();
		DrawDebugDirectionalArrow(
			GetWorld(),
			DebugStart,
			DebugStart + StrafeForce.GetSafeNormal() * 150.0f,
			15.0f,
			FColor::Yellow,
			false,
			-1.0f,
			0,
			2.0f
		);
	}
}

void UHoverMovementComponent::ApplyDrag(float DeltaTime)
{
	UPrimitiveComponent* PhysComp = GetPhysicsComponent();
	if (!PhysComp || !PhysComp->IsSimulatingPhysics())
	{
		return;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	// Apply linear drag
	if (LinearDrag > 0.0f)
	{
		FVector Velocity = PhysComp->GetPhysicsLinearVelocity();
		FVector ForwardVector = Owner->GetActorForwardVector();
		FVector RightVector = Owner->GetActorRightVector();

		// Get forward and lateral components
		float ForwardSpeed = FVector::DotProduct(Velocity, ForwardVector);
		float LateralSpeed = FVector::DotProduct(Velocity, RightVector);

		// Apply drag force opposing motion
		FVector DragForce = FVector::ZeroVector;

		// Stronger lateral drag for better handling
		DragForce -= ForwardVector * ForwardSpeed * LinearDrag;
		DragForce -= RightVector * LateralSpeed * LinearDrag * 2.0f; // More lateral drag

		PhysComp->AddForce(DragForce);
	}

	// Apply angular drag
	if (AngularDrag > 0.0f)
	{
		FVector AngularVelocity = PhysComp->GetPhysicsAngularVelocityInRadians();
		FVector AngularDragTorque = -AngularVelocity * AngularDrag * 1000.0f;
		PhysComp->AddTorqueInRadians(AngularDragTorque);
	}
}

UPrimitiveComponent* UHoverMovementComponent::GetPhysicsComponent() const
{
	if (CachedPhysicsComponent)
	{
		return CachedPhysicsComponent;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	return Cast<UPrimitiveComponent>(Owner->GetRootComponent());
}

float UHoverMovementComponent::GetSpeedBasedTurnMultiplier() const
{
	if (!bSpeedAffectsTurning)
	{
		return 1.0f;
	}

	float CurrentSpeed = FMath::Abs(GetForwardSpeed());

	if (CurrentSpeed >= FullTurnSpeed)
	{
		return 1.0f;
	}

	// Interpolate between min multiplier at rest and full at FullTurnSpeed
	float T = CurrentSpeed / FullTurnSpeed;
	return FMath::Lerp(MinTurnMultiplierAtRest, 1.0f, T);
}
