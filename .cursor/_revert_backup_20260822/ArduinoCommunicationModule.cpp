// Arduino Communication Plugin - Module Implementation

#include "ArduinoCommunicationModule.h"
#include "ShipHardwareInputComponent.h"
#include "WeaponImuLog.h"
#include "HAL/IConsoleManager.h"

#define LOCTEXT_NAMESPACE "FArduinoCommunicationModule"

namespace
{
	uint8 ParseWeaponImuSideArg(const TArray<FString>& Args, bool& bOk)
	{
		bOk = false;
		if (Args.Num() < 1)
		{
			return 0;
		}

		const FString Side = Args[0].ToLower();
		if (Side == TEXT("port") || Side == TEXT("left") || Side == TEXT("0"))
		{
			bOk = true;
			return 0;
		}
		if (Side == TEXT("starboard") || Side == TEXT("stbd") || Side == TEXT("right") || Side == TEXT("1"))
		{
			bOk = true;
			return 1;
		}
		return 0;
	}

	void LogWeaponImuIsolationUsage()
	{
		UE_LOG(LogWeaponImu, Warning, TEXT("Usage: WeaponImu.Sweep Port|Starboard   or   WeaponImu.Nudge Port|Starboard"));
	}
}

void FArduinoCommunicationModule::StartupModule()
{
	UE_LOG(LogTemp, Log, TEXT("ArduinoCommunication: Module started"));

	SweepCommand = IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("WeaponImu.Sweep"),
		TEXT("Isolation test: sweep Port or Starboard gun aim for 5s without hardware. Usage: WeaponImu.Sweep Port"),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
		{
			bool bOk = false;
			const uint8 Side = ParseWeaponImuSideArg(Args, bOk);
			if (!bOk)
			{
				LogWeaponImuIsolationUsage();
				return;
			}
			if (UShipHardwareInputComponent* Comp = UShipHardwareInputComponent::FindPrimaryInPlayWorld())
			{
				Comp->StartAimIsolationSweep(Side);
			}
		}),
		ECVF_Default);

	NudgeCommand = IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("WeaponImu.Nudge"),
		TEXT("Isolation test: snap Port or Starboard gun to +35 yaw. Usage: WeaponImu.Nudge Port"),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
		{
			bool bOk = false;
			const uint8 Side = ParseWeaponImuSideArg(Args, bOk);
			if (!bOk)
			{
				LogWeaponImuIsolationUsage();
				return;
			}
			if (UShipHardwareInputComponent* Comp = UShipHardwareInputComponent::FindPrimaryInPlayWorld())
			{
				Comp->ApplyAimIsolationNudge(Side);
			}
		}),
		ECVF_Default);

	FireBurstCommand = IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("WeaponFire.Burst"),
		TEXT("Isolation test: force SetFiring(true) on Port or Starboard. Usage: WeaponFire.Burst Port"),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
		{
			bool bOk = false;
			const uint8 Side = ParseWeaponImuSideArg(Args, bOk);
			if (!bOk)
			{
				UE_LOG(LogWeaponImu, Warning, TEXT("Usage: WeaponFire.Burst Port|Starboard"));
				return;
			}
			if (UShipHardwareInputComponent* Comp = UShipHardwareInputComponent::FindPrimaryInPlayWorld())
			{
				Comp->ApplyFireIsolationBurst(Side);
			}
		}),
		ECVF_Default);
}

void FArduinoCommunicationModule::ShutdownModule()
{
	if (SweepCommand)
	{
		IConsoleManager::Get().UnregisterConsoleObject(SweepCommand);
		SweepCommand = nullptr;
	}
	if (NudgeCommand)
	{
		IConsoleManager::Get().UnregisterConsoleObject(NudgeCommand);
		NudgeCommand = nullptr;
	}
	if (FireBurstCommand)
	{
		IConsoleManager::Get().UnregisterConsoleObject(FireBurstCommand);
		FireBurstCommand = nullptr;
	}
	UE_LOG(LogTemp, Log, TEXT("ArduinoCommunication: Module shutdown"));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FArduinoCommunicationModule, ArduinoCommunication)
