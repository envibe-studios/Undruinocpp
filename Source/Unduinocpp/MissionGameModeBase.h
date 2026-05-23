// Mission Game Mode Base - Optional C++ GameMode that wires MissionManager to the session
// Sets Game State to AMissionGameState and registers missions from a registry asset.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MissionManagerSubsystem.h"
#include "MissionGameModeBase.generated.h"

/**
 * GameMode base that configures the mission system for the session.
 * Set MissionRegistry in the editor; on BeginPlay the main mission is started.
 * You can use this as your GameMode or set your existing GameMode's Game State Class to AMissionGameState
 * and call RegisterMissionsFromRegistry from Blueprint or C++.
 */
UCLASS(Blueprintable)
class UNDUINOCPP_API AMissionGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMissionGameModeBase();

	virtual void BeginPlay() override;

	/** Registry of missions for this session. Set in editor; used on BeginPlay to register and start the main mission. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission")
	TObjectPtr<UMissionRegistryAsset> MissionRegistry;
};
