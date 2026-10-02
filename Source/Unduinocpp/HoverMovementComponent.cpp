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

	BoostEnergy = MaxBoostEnergy;
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

	// Dual-stick / other external analog: win at apply time (survives mid-frame Set*Input(0)).
	if (bExternalAnalogActive)
	{
		RawThrottleInput = ExternalThrottleInput;
		RawSteeringInput = ExternalSteeringInput;
		if (bEnableStrafe)
		{
			RawStrafeInput = ExternalStrafeInput;
		}
	}

	// Update smoothed input values
	UpdateInputSmoothing(DeltaTime);

	UpdateBoost(DeltaTime);

	// Apply forces
	ApplyThrust(DeltaTime);
	ApplyTurning(DeltaTime);
	ApplyTurnBank(DeltaTime);

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

void UHoverMovementComponent::SetExternalAnalogInput(float Throttle, float Steering, float Strafe, bool bActive)
{
	bExternalAnalogActive = bActive;
	ExternalThrottleInput = FMath::Clamp(Throttle, -1.0f, 1.0f);
	ExternalSteeringInput = FMath::Clamp(Steering, -1.0f, 1.0f);
	ExternalStrafeInput = FMath::Clamp(Strafe, -1.0f, 1.0f);

	if (bActive)
	{
		RawThrottleInput = ExternalThrottleInput;
		RawSteeringInput = ExternalSteeringInput;
		if (bEnableStrafe)
		{
			RawStrafeInput = ExternalStrafeInput;
		}
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

void UHoverMovementComponent::SetBoostInput(bool bPressed)
{
	bBoostInputHeld = bPressed;
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

float UHoverMovementComponent::GetCurrentBankAmount() const
{
	return CurrentBankAmount;
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

bool UHoverMovementComponent::IsBoostActive() const
{
	return bBoostActive;
}

float UHoverMovementComponent::GetBoostEnergy() const
{
	return BoostEnergy;
}

float UHoverMovementComponent::GetBoostEnergyNormalized() const
{
	if (MaxBoostEnergy <= KINDA_SMALL_NUMBER)
	{
		return 0.0f;
	}
	return FMath::Clamp(BoostEnergy / MaxBoostEnergy, 0.0f, 1.0f);
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
		ClearTurnBankOffsets();
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
	CurrentBankAmount = 0.0f;

	bForwardPressed = false;
	bBackwardPressed = false;
	bLeftPressed = false;
	bRightPressed = false;
	bStrafeLeftPressed = false;
	bStrafeRightPressed = false;

	bBoostInputHeld = false;
	SetBoostActive(false);
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

void UHoverMovementComponent::UpdateBoost(float DeltaTime)
{
	if (!bEnableBoost)
	{
		if (bBoostActive)
		{
			SetBoostActive(false);
		}
		return;
	}

	const float ClampedMax = FMath::Max(MaxBoostEnergy, KINDA_SMALL_NUMBER);
	const float PreviousEnergy = BoostEnergy;

	bool bWantsBoost = bBoostInputHeld;
	if (bWantsBoost)
	{
		if (!bBoostActive)
		{
			// Fresh start requires MinEnergyToStart; continuing hold after empty stays off until release+threshold
			bWantsBoost = BoostEnergy >= MinEnergyToStart;
		}
		else
		{
			bWantsBoost = BoostEnergy > 0.0f;
		}
	}

	SetBoostActive(bWantsBoost);

	if (bBoostActive)
	{
		BoostEnergy = FMath::Max(0.0f, BoostEnergy - BoostDrainRate * DeltaTime);
		if (BoostEnergy <= 0.0f)
		{
			SetBoostActive(false);
		}
	}
	else
	{
		BoostEnergy = FMath::Min(ClampedMax, BoostEnergy + BoostRechargeRate * DeltaTime);
	}

	if (!FMath::IsNearlyEqual(PreviousEnergy, BoostEnergy))
	{
		OnBoostEnergyChanged.Broadcast(BoostEnergy);
	}
}

void UHoverMovementComponent::SetBoostActive(bool bNewActive)
{
	if (bBoostActive == bNewActive)
	{
		return;
	}

	bBoostActive = bNewActive;
	OnBoostActiveChanged.Broadcast(bBoostActive);
}

void UHoverMovementComponent::ApplyThrust(float DeltaTime)
{
	float EffectiveThrottle = CurrentThrottle;
	if (bBoostActive && EffectiveThrottle >= 0.0f)
	{
		// Idle boost: Shift alone (or light forward throttle) still accelerates at full forward
		EffectiveThrottle = FMath::Max(EffectiveThrottle, 1.0f);
	}

	if (FMath::IsNearlyZero(EffectiveThrottle))
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
	float ThrustMagnitude = (EffectiveThrottle > 0.0f)
		? EffectiveThrottle * MaxForwardThrust
		: EffectiveThrottle * MaxBackwardThrust; // Note: EffectiveThrottle is negative here

	if (bBoostActive && EffectiveThrottle > 0.0f)
	{
		ThrustMagnitude *= BoostThrustMultiplier;
	}

	// Thrust along heading in the ground plane so bank lean doesn't aim thrust skyward/into the pad
	const FVector TurnAxis = GetTurnAxis();
	FVector ForwardVector = FVector::VectorPlaneProject(Owner->GetActorForwardVector(), TurnAxis).GetSafeNormal();
	if (ForwardVector.IsNearlyZero())
	{
		ForwardVector = Owner->GetActorForwardVector();
	}
	const FVector ThrustForce = ForwardVector * ThrustMagnitude;

	// Apply through center of mass. Any vertical lever arm while banked turns actor-pitch into world-yaw
	// and steers opposite the turn under hard acceleration (even when ThrustHeightOffset is 0, if COM != actor location).
	PhysComp->AddForce(ThrustForce);

	// Optional nose pitch from ThrustHeightOffset, constrained to the ground-plane right axis so bank can't convert it to yaw
	if (!FMath::IsNearlyZero(ThrustHeightOffset))
	{
		const FVector PitchAxis = FVector::CrossProduct(TurnAxis, ForwardVector).GetSafeNormal();
		if (!PitchAxis.IsNearlyZero())
		{
			// Matches old Cross(Up * Offset, Forward * F) magnitude/sign, but around level right instead of actor-right
			const float PitchTorque = -ThrustHeightOffset * ThrustMagnitude;
			PhysComp->AddTorqueInRadians(PitchAxis * PitchTorque);
		}
	}

	// Debug visualization
	if (bDrawDebug)
	{
		const FVector DebugStart = PhysComp->GetCenterOfMass();
		const FColor ArrowColor = bBoostActive
			? FColor::Cyan
			: (EffectiveThrottle > 0.0f ? FColor::Green : FColor::Red);
		DrawDebugDirectionalArrow(
			GetWorld(),
			DebugStart,
			DebugStart + ThrustForce.GetSafeNormal() * 200.0f,
			20.0f,
			ArrowColor,
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

	// Yaw around ground/world up — NOT actor up.
	// When turn-bank rolls the craft, actor-up yaw injects pitch torque that thrusters fight,
	// which feels like the ship steers back to straight after the initial turn.
	const FVector TurnAxis = GetTurnAxis();
	const FVector Torque = TurnAxis * TorqueMagnitude;

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

void UHoverMovementComponent::ApplyTurnBank(float DeltaTime)
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	if (!bEnableTurnBank || MaxBankHeightOffset <= 0.0f || RegisteredThrusters.Num() == 0)
	{
		if (!FMath::IsNearlyZero(CurrentBankAmount))
		{
			CurrentBankAmount = 0.0f;
			ClearTurnBankOffsets();
		}
		return;
	}

	const float TargetBank = CurrentSteering * GetSpeedBasedBankMultiplier();
	CurrentBankAmount = FMath::FInterpTo(CurrentBankAmount, TargetBank, DeltaTime, BankResponseSpeed);

	if (FMath::IsNearlyZero(CurrentBankAmount, KINDA_SMALL_NUMBER))
	{
		CurrentBankAmount = 0.0f;
		ClearTurnBankOffsets();
		return;
	}

	// Determine lateral span so center thrusters bank less than outer corners
	float MaxLateralAbs = 0.0f;
	for (UHoverThrusterComponent* Thruster : RegisteredThrusters)
	{
		if (!Thruster)
		{
			continue;
		}

		const FVector LocalPos = Owner->GetActorTransform().InverseTransformPosition(Thruster->GetComponentLocation());
		MaxLateralAbs = FMath::Max(MaxLateralAbs, FMath::Abs(LocalPos.Y));
	}

	if (MaxLateralAbs <= KINDA_SMALL_NUMBER)
	{
		ClearTurnBankOffsets();
		return;
	}

	const float BankHeight = CurrentBankAmount * MaxBankHeightOffset;

	for (UHoverThrusterComponent* Thruster : RegisteredThrusters)
	{
		if (!Thruster)
		{
			continue;
		}

		const FVector LocalPos = Owner->GetActorTransform().InverseTransformPosition(Thruster->GetComponentLocation());
		const float LateralNorm = FMath::Clamp(LocalPos.Y / MaxLateralAbs, -1.0f, 1.0f);

		// Positive steering / bank = right turn: lower right thrusters, raise left thrusters (bank into turn)
		const float HeightOffset = -LateralNorm * BankHeight;
		Thruster->SetHoverHeightOffset(HeightOffset);

		if (bDrawDebug)
		{
			const FVector ThrusterLoc = Thruster->GetComponentLocation();
			DrawDebugLine(
				GetWorld(),
				ThrusterLoc,
				ThrusterLoc + FVector(0.0f, 0.0f, HeightOffset),
				HeightOffset >= 0.0f ? FColor::Cyan : FColor::Orange,
				false,
				-1.0f,
				0,
				2.0f
			);
		}
	}

	// Optional roll assist — thruster height bias is the primary bank mechanism
	if (BankAssistTorque > 0.0f)
	{
		UPrimitiveComponent* PhysComp = GetPhysicsComponent();
		if (PhysComp && PhysComp->IsSimulatingPhysics())
		{
			// UE: positive roll torque around forward tips right side down
			const FVector RollTorque = Owner->GetActorForwardVector() * (CurrentBankAmount * BankAssistTorque);
			PhysComp->AddTorqueInRadians(RollTorque);
		}
	}
}

void UHoverMovementComponent::ClearTurnBankOffsets()
{
	for (UHoverThrusterComponent* Thruster : RegisteredThrusters)
	{
		if (Thruster)
		{
			Thruster->SetHoverHeightOffset(0.0f);
		}
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

	// Apply linear drag in the ground plane so bank lean doesn't tilt drag into thruster axes
	if (LinearDrag > 0.0f)
	{
		const FVector Velocity = PhysComp->GetPhysicsLinearVelocity();
		const FVector TurnAxis = GetTurnAxis();

		FVector ForwardVector = FVector::VectorPlaneProject(Owner->GetActorForwardVector(), TurnAxis).GetSafeNormal();
		if (ForwardVector.IsNearlyZero())
		{
			ForwardVector = Owner->GetActorForwardVector();
		}

		FVector RightVector = FVector::CrossProduct(TurnAxis, ForwardVector).GetSafeNormal();
		if (RightVector.IsNearlyZero())
		{
			RightVector = FVector::VectorPlaneProject(Owner->GetActorRightVector(), TurnAxis).GetSafeNormal();
		}

		const float ForwardSpeed = FVector::DotProduct(Velocity, ForwardVector);
		const float LateralSpeed = FVector::DotProduct(Velocity, RightVector);

		FVector DragForce = FVector::ZeroVector;
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

float UHoverMovementComponent::GetSpeedBasedBankMultiplier() const
{
	if (!bScaleBankWithSpeed)
	{
		return 1.0f;
	}

	const float CurrentSpeed = FMath::Abs(GetForwardSpeed());
	if (CurrentSpeed >= FullBankSpeed)
	{
		return 1.0f;
	}

	const float T = CurrentSpeed / FullBankSpeed;
	return FMath::Lerp(MinBankMultiplierAtRest, 1.0f, T);
}

FVector UHoverMovementComponent::GetTurnAxis() const
{
	FVector AccumulatedNormal = FVector::ZeroVector;
	int32 NormalCount = 0;

	for (const UHoverThrusterComponent* Thruster : RegisteredThrusters)
	{
		if (Thruster && Thruster->IsGroundDetected())
		{
			AccumulatedNormal += Thruster->GetGroundNormal();
			++NormalCount;
		}
	}

	if (NormalCount > 0)
	{
		const FVector AveragedNormal = AccumulatedNormal.GetSafeNormal();
		if (!AveragedNormal.IsNearlyZero())
		{
			return AveragedNormal;
		}
	}

	return FVector::UpVector;
}
