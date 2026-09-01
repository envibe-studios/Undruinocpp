// Arduino Communication Plugin - Module Header

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class IConsoleObject;

class FArduinoCommunicationModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	IConsoleObject* SweepCommand = nullptr;
	IConsoleObject* NudgeCommand = nullptr;
	IConsoleObject* FireBurstCommand = nullptr;
};
