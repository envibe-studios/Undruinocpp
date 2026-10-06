// Copyright Epic Games, Inc. All Rights Reserved.

#include "ScreenBridge.h"
#include "ScreenBridgeBPLibrary.h"
#include "Engine/World.h"

#define LOCTEXT_NAMESPACE "FScreenBridgeModule"

void FScreenBridgeModule::StartupModule()
{
	WorldCleanupHandle = FWorldDelegates::OnWorldCleanup.AddLambda(
		[](UWorld* World, bool, bool)
		{
			UScreenBridgeBPLibrary::CloseWindowsForWorld(World);
		});
}

void FScreenBridgeModule::ShutdownModule()
{
	FWorldDelegates::OnWorldCleanup.Remove(WorldCleanupHandle);
	UScreenBridgeBPLibrary::CloseAllWindows();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FScreenBridgeModule, ScreenBridge)
