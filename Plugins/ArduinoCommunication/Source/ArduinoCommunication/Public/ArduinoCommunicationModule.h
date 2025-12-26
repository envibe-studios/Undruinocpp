// Arduino Communication Plugin - Module Header

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FArduinoCommunicationModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
