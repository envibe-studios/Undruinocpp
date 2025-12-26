// Arduino Communication Plugin - Module Implementation

#include "ArduinoCommunicationModule.h"

#define LOCTEXT_NAMESPACE "FArduinoCommunicationModule"

void FArduinoCommunicationModule::StartupModule()
{
	UE_LOG(LogTemp, Log, TEXT("ArduinoCommunication: Module started"));
}

void FArduinoCommunicationModule::ShutdownModule()
{
	UE_LOG(LogTemp, Log, TEXT("ArduinoCommunication: Module shutdown"));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FArduinoCommunicationModule, ArduinoCommunication)
