# Unduinocpp — Project Architecture

Documentation for AI agents and engineers joining this Unreal Engine project.

**Scope:** Verified from source, configs, plugin descriptors, and Content asset names present in the repository. Speculative or outdated statements from older README/`CLAUDE.md` text are called out where they conflict with project files.

**Do not treat older root docs as authoritative for engine version** — see [Engine version note](#engine-version-note).

---

## 1. Project identity

Unduinocpp is an Unreal Engine game project centered on a **physical spaceship bridge simulator**: Unreal gameplay systems plus physical controls/displays driven by ESP32 hardware (serial, ESP-NOW on the hardware side, diagnostics).

The repository also contains a reusable **ArduinoCommunication** plugin for UE ↔ ESP32/ESP8266 communication. Gameplay systems (hovercraft, missions, enemy AI, weapons) live primarily in the `Unduinocpp` game module and Content Blueprints.

Project identity for agents is also stated in `.cursor/rules/00-project-core.mdc`.

---

## 2. Engine version note

| Source | Stated version |
|--------|----------------|
| `Unduinocpp.uproject` → `EngineAssociation` | **5.8** |
| `Source/Unduinocpp.Target.cs` → `IncludeOrderVersion` | **Unreal5_8** |
| Plugin descriptors (`ArduinoCommunication`, `UnrealMCP`, `ScreenBridge`) → `EngineVersion` | **5.8.0** |
| Root `README.md` / `CLAUDE.md` | Still mention **5.7** in places |

**Authoritative for this checkout:** Unreal Engine **5.8** (from `.uproject` and Target settings). Treat 5.7 mentions in older markdown as stale unless updated.

---

## 3. Top-level layout

```
/
├── Unduinocpp.uproject          # Module + enabled plugins
├── Source/
│   ├── Unduinocpp/              # Runtime game module (C++)
│   ├── Unduinocpp.Target.cs
│   └── UnduinocppEditor.Target.cs
├── Plugins/
│   ├── ArduinoCommunication/    # Hardware / Andy / serial (project plugin)
│   ├── UnrealMCP/               # Editor MCP bridge (project plugin)
│   └── ScreenBridge/            # Multi-window display plugin (Fab/marketplace)
├── Content/                     # Maps, Blueprints, art, Fab assets
├── Config/                      # Default*.ini
├── Tools/                       # Utility scripts (e.g. Gaea partition helpers)
├── MISSION_SYSTEM_GUIDE.md      # Designer-facing mission system docs
├── CLAUDE.md / README.md        # Older plugin-focused overviews (may be stale)
└── .cursor/
    ├── rules/00-project-core.mdc
    └── docs/                    # This document and future agent docs
```

Other folders (`Binaries/`, `Intermediate/`, `Saved/`, `DerivedDataCache/`) are build/editor artifacts.

---

## 4. Modules and build

### 4.1 Game module: `Unduinocpp`

- **Type:** Runtime (`Unduinocpp.uproject`)
- **Build file:** `Source/Unduinocpp/Unduinocpp.Build.cs`
- **Public dependencies:** Core, CoreUObject, Engine, InputCore, EnhancedInput, NetCore, AIModule, GameplayTasks, NavigationSystem, GameplayTags
- **Private dependencies:** Sockets, Networking
- **Include paths:** `ModuleDirectory` is added as a public include path so headers can use `#include "AI/..."` style paths

Targets:

- `Unduinocpp.Target.cs` — Game target, `BuildSettingsVersion.V7`, `EngineIncludeOrderVersion.Unreal5_8`
- `UnduinocppEditor.Target.cs` — Editor target

### 4.2 Important module coupling

`ArduinoCommunication.Build.cs` lists **`Unduinocpp` as a public dependency** and adds `Source/Unduinocpp` as a private include path. The plugin uses game types such as `UFiringComponent` / `EFiringModeType` (e.g. `WeaponMag.h`).

Implication for agents: changes to weapon/firing types in the game module can affect the Arduino plugin; the plugin is not fully isolated from game code.

---

## 5. Enabled plugins (`Unduinocpp.uproject`)

### 5.1 Project-local plugins (under `Plugins/`)

| Plugin | Type | Role (from descriptors / source) |
|--------|------|----------------------------------|
| **ArduinoCommunication** | Runtime | Serial/TCP Arduino/ESP communication; Andy serial hub; ship hardware input; diagnostics UI helpers; multi-display camera component |
| **UnrealMCP** | Editor | Model Context Protocol bridge for Unreal Editor (TCP JSON commands). Depends on EditorScriptingUtilities |
| **ScreenBridge** | Runtime (Win64) | Multi-window / multi-screen experiences (Fab product). Module startup/shutdown is empty in the checked-in stub |

### 5.2 Other plugins listed in `.uproject`

Enabled (engine/marketplace plugins; not present as source under this repo’s `Plugins/` folder unless installed elsewhere):

- ModelingToolsEditorMode (Editor only)
- MeshPartition, MeshPartitionWater, MeshTerrainMode, PCGMeshPartitionInterop
- ModelContextProtocol, Terminal, EditorToolset, AllToolsets

Disabled:

- GaeaUnrealTools

---

## 6. Default maps and GameMode

From `Config/DefaultEngine.ini` `[/Script/EngineSettings.GameMapsSettings]`:

| Setting | Value |
|---------|--------|
| GameDefaultMap | `/Game/Level/TestLevel.TestLevel` |
| EditorStartupMap | `/Game/Level/HoverHandlingCourse.HoverHandlingCourse` |
| GlobalDefaultGameMode | `/Game/Blueprints/GameMode_ShipPorts.GameMode_ShipPorts_C` |

Maps present under `Content/Level/` (asset names): `TestLevel`, `HoverTrack`, `HoverHandlingCourse`, `EnemyTest`, `GaeaCanyons`, `TestMeshTerrain`.

`HoverHandlingCourse` is a dedicated hovercraft handling/tuning circuit (speed runway, wide sweep, chicane, hairpin, slalom, freeplay apron). GameMode default pawn is `BP_Hovercraft`.

---

## 7. Major gameplay systems (C++)

All paths below are under `Source/Unduinocpp/` unless noted.

### 7.1 Hovercraft / vehicle

| Class | Kind | Notes |
|-------|------|--------|
| `UHoverThrusterComponent` | SceneComponent | Hover lift via ground traces; runtime `HoverHeightOffset` for attitude shaping; thruster hitpoints / health states; sputter malfunctions; replicated hitpoints |
| `UHoverMovementComponent` | ActorComponent | Throttle/steering/strafe input; applies physics thrust/yaw torque; turn-bank via differential thruster height bias; hold-to-boost energy pool (`SetBoostInput`); auto-registers thrusters |
| `UDualJoystickTankInputComponent` | ActorComponent | Optional dual DirectInput sticks (e.g. two Logitech Attack 3) via Windows Joy0/Joy1; tank-mix Forward/Yaw + averaged Strafe into `UHoverMovementComponent`; bypasses Gamepad merging for identical USB devices |

Class groups: `Vehicle`. Designed for analog hardware (pedals, wheels, dual joysticks) and digital keyboard input.

**Dual Attack 3 tank sticks (optional):** Add `UDualJoystickTankInputComponent` to `BP_Hovercraft`. It polls winmm joystick IDs (default Left=0, Right=1), applies per-axis invert/deadzone/curve, mixes `Forward=(LY+RY)/2`, `Yaw=(LY-RY)/2`, `Strafe=(LX+RX)/2`, and drives hover via `UHoverMovementComponent::SetExternalAnalogInput` (latched at movement Tick so ESP/`SetThrottle(0)` cannot wipe stick input mid-frame). Boost: both sticks' button 3 must be held (`SetBoostInput`); releasing either cancels. Keyboard and `BPI_ESPComm` resume when sticks are near center / boost buttons released. Toggle on-screen debug with **F8** (`bDebugDisplay`). If left/right devices are swapped, use `bSwapLeftRightDevices`. Identical Attack 3 units are **not** reliably separable via Unreal Gamepad/Enhanced Input axes — use Joy IDs.

Turn-bank (enabled by default): steering biases left/right thruster `HoverHeightOffset` so outer corners rise and inner corners settle, producing a physics lean into the turn through the existing hover springs instead of fighting thruster angular damping with roll torque. Tunables live under `Hover Movement|Turn Bank` (`MaxBankHeightOffset`, `BankResponseSpeed`, optional speed scaling / assist torque).

Boost / turbo (enabled by default): `SetBoostInput(bool)` hold semantics for keyboard Left Shift (`ActionMappings` `Boost`) and future ESP32 start/end via `BPI_ESPComm::SetBoost`. Energy drains while active and recharges when idle; tunables under `Hover Movement|Boost` (`MaxBoostEnergy`, `BoostDrainRate`, `BoostRechargeRate`, `MinEnergyToStart`, `BoostThrustMultiplier`).

### 7.2 Weapons / combat (player & turrets)

| Class | Kind | Notes |
|-------|------|--------|
| `UFiringComponent` | SceneComponent | Modes: Bullet, TractorBeam, Scanner, Custom; IMU aim; weapon-mag apply |
| `UStationaryTurretComponent` | ActorComponent | 2-axis aim (yaw/pitch), projectile spawn, leading; optional Enemy AI blackboard targeting + ship-part (thruster/Engine) aim priority |
| `UTurretHealthComponent` | ActorComponent | Legacy non-AI turret HP/explosion (kept for compatibility; AI emplacements use `UEnemyHealthComponent`) |
| `UMiniCRTWeaponDisplayComponent` | ActorComponent | Sends `!crt,...` ASCII commands via `UAndySerialSubsystem` for MiniCRT displays |

Tractor beam mode in `UFiringComponent` performs **trace + delegates only** (no physics pull in C++). Presentation (beams, tracers) is expected on those delegates in content, not in the firing core.

### 7.3 Mission / threat system

Documented in detail in `MISSION_SYSTEM_GUIDE.md`. Core types:

| Class | Kind | Role |
|-------|------|------|
| `UMissionDataAsset` | PrimaryDataAsset | One mission definition (ID, type, objectives, visibility, fail-forward actions) |
| `UMissionRegistryAsset` | PrimaryDataAsset | Session mission list + `MainMissionID` + threat thresholds (declared in `MissionManagerSubsystem.h`) |
| `UMissionManagerSubsystem` | GameInstanceSubsystem + tickable | Registers missions, server-authoritative progress/threat, UI summary helpers, world-event delegates |
| `AMissionGameState` | GameState | Replicated threat + mission states + session objective progress |
| `AMissionPlayerState` | PlayerState | Replicated per-player objective progress |
| `AMissionGameModeBase` | GameModeBase | Sets GameState/PlayerState classes; calls `RegisterMissionsFromRegistry` on BeginPlay |
| `MissionTypes.h` | Enums/structs | States, roles (Pilot/Gunner/Engineer), objective types, visibility/actions |

**Networking model (missions):** Server writes via subsystem; clients read replicated GameState / PlayerState. Session vs PerPlayer objective scopes.

**Fail-forward design:** Failure/expiry can raise threat and unlock further missions via data-driven actions (see guide + headers).

Blueprint mission content under `Content/Blueprints/Missions/` includes e.g. `DA_MissionRegistry`, `DA_MissionTest1/2`, `BP_Waypoint`, `BP_Destroyable`, `BPComp_MissionCompleter`, UI widgets for objectives.

### 7.4 Enemy AI

Folder: `Source/Unduinocpp/AI/` (+ `AI/BT/`).

| Piece | Role |
|-------|------|
| `AEnemyPawn` | Capsule + mesh + Health / Movement / Ability / Aggro components; `UEnemyDefinition`; squad id/role (replicated) |
| `AEnemyTurretPawn` | Stationary emplacement subclass of `AEnemyPawn`; preserves `DamageCollider` hit volume; drives `UStationaryTurretComponent` from Enemy AI targets |
| `AEnemyAIController` | Perception (sight), Behavior Tree, target policy, LOD tick intervals, synergy assignment helpers; `bAutoPursueTarget` from definition (false for turrets) |
| `UEnemyDefinition` | PrimaryDataAsset: BT, blackboard, movement mode class, `bAutoPursueTarget`, abilities, perception/aggro params, health, squad role |
| `UEnemyMovementComponent` + modes | Abstract `UEnemyMovementMode`; concrete: Flying (approach/orbit/dive-bomb thrusters), Floating, Crawling, Burrowing, **Stationary** (no translation). Combat focus actor drives flyer pathing (not a beeline). |
| `UEnemyAbility` / `UEnemyAbilityLoadout` / `UEnemyAbilityComponent` | Lightweight ability system |
| `UAggroComponent` / `UEnemyHealthComponent` | Threat table / HP (turret death FX/explosion via health component) |
| `AEnemySpawner` | Spawn from definition; optional pooling; squad id |
| `ASquadCoordinator` | Assigns Carry/Drop-style synergy to Carrier + Payload pairs via blackboard keys |
| `UEnemyAIDirectorSubsystem` | WorldSubsystem; binds mission threat events and scales aggression / ability cooldown |

Shared enums and blackboard key names: `AI/EnemyTypes.h` (`FEnemyBlackboardKeys`, combat/movement/squad/synergy enums).

C++ Behavior Tree nodes under `AI/BT/`:

- Tasks: Idle, MoveTo, FaceTarget, UseAbility, FlockOffset, BurrowRelocate, ExecuteSynergy
- Service: UpdateEnemyTarget
- Decorator: CanUseAbility

Blueprint AI assets under `Content/Blueprints/EnemyAI/` (e.g. `BT_Enemy_Basic`, `BB_Enemy`, definitions including `DA_EnemyDefinition_Turret`, example BPs / BP tasks).

**Stationary turret Content:** C++ supports `AEnemyTurretPawn` + `DA_EnemyDefinition_Turret`, but `BP_Turret` was restored to its pre-migration `Actor` parent after a Live-Coding reparent left a NULL parent / broken SCS. Prefer a **new** EnemyTurret Blueprint after a full editor module rebuild (not Live Coding-only) rather than reparenting the production Actor asset in-place.

### 7.5 External display networking (game module)

| Class | Protocol | Purpose |
|-------|----------|---------|
| `UMinimapUDPSenderComponent` | UDP plaintext `"X,Y,Yaw"` | Stream pose to an external minimap (e.g. Raspberry Pi); default IP/port in header |

Depends on Sockets/Networking (module private deps).

### 7.6 Placeholder / unused-looking stubs

`MyClass` / `MyClass2` exist as minimal C++ stubs — not part of the systems above.

---

## 8. Hardware / Andy architecture (ArduinoCommunication plugin)

Path: `Plugins/ArduinoCommunication/`.

### 8.1 Layers

```
ESP nodes (ESP-NOW) → Andy hub (serial USB) → Unreal
                                              ├─ UAndySerialSubsystem (multi-port, GameInstance)
                                              │    └─ UByteStreamPacketParser (binary frames)
                                              ├─ UShipHardwareInputComponent (filter by ShipId → BP events)
                                              ├─ UAndyDiagSubsystem (!diag,quick / DIAG_* lines)
                                              ├─ UMiniCRTWeaponDisplayComponent (!crt commands, game module)
                                              └─ UArduinoCommunicationComponent (legacy text Serial/TCP API)
```

### 8.2 Binary frame protocol

Documented in `ByteStreamPacketParser.h`:

```
[0xAA][VER][SRC][TYPE][SEQ_L][SEQ_H][LEN][PAYLOAD 0..32][CRC][0x55]
```

CRC: XOR of bytes from VER through end of payload.

### 8.3 Message types (`EEspMsgType` in `EspPacketBP.h`)

| Value | Name |
|------:|------|
| 1 | WheelTurn |
| 2 | RepairProgress |
| 3 | JackState |
| 4 | WeaponTag |
| 5 | ReloadTag |
| 6 | WeaponImu |

Payload helpers live in `UEspPacketBP` (Blueprint function library).

### 8.4 Key plugin classes

| Class | Role |
|-------|------|
| `UArduinoSerialPort` / `UArduinoTcpClient` | Low-level I/O |
| `UArduinoCommunicationComponent` | ActorComponent: Serial or TCP, line/byte events |
| `UAndySerialSubsystem` | Multi-port hub by `ShipId`; `OnFrameParsed`, `OnConnectionChanged`, `OnLineReceived` |
| `UShipHardwareInputComponent` | Ship-scoped hardware events; weapon mags → `UFiringComponent` (weapon bay only); IMU routing Port/Starboard; forwards aim via `SendWeaponAim(Port,Quat,TriggerDown)` and fire via `SendFire`; reload bay (`ReaderIndex` 2) occupancy + UID-0 remove attribution |
| `UByteStreamPacketParser` / `UPacketParserComponent` | Framing |
| `UAndyDiagSubsystem` / `UAndyDiagRegistry` / widgets | Quick diagnostics over serial (`!diag,quick`) |
| `UWeaponMag` / `UWeaponMagDataAsset` | RFID tag → firing config |
| `UMultiDisplayCameraComponent` | Scene capture → OS window on chosen monitor |
| `AArduinoConnectionTestActor` | Connection test helper |
| `UArduinoBlueprintLibrary` | BP helpers |

### 8.5 Default Andy diagnostic roll call (10 nodes)

Canonical IDs from `AndyDiagRegistry.cpp`:

| ID | Display label |
|----|----------------|
| ANDY | Andy |
| PORT | Weapon Port |
| WPN_STBD | Weapon Starboard |
| ENG_FL / ENG_FR / ENG_RL / ENG_RR | Engine corners |
| ENG_MAIN | Engine Main |
| DISP_PORT / DISP_STBD | Weapon Port/Starboard Display |

Aliases exist (e.g. `WEAPON_PORT` → `PORT`, `ENGINE_FRONT_LEFT` → `ENG_FL`).

### 8.6 MiniCRT outbound command (game module)

From `MiniCRTWeaponDisplayComponent.h`:

```
!crt,{Side},{CurrentAmmo},{MaxAmmo},{FireMode},{Reloading}\n
```

Side: `0` = Port, `1` = Starboard. Routed through existing Andy serial (`ShipId` must match `AddPort`).

### 8.7 Arduino sketches

Under `Plugins/ArduinoCommunication/ArduinoSketches/`:

- `ESP8266_Serial_Communication/`
- `ESP8266_WiFi_Communication/`
- `ESP32_Serial_Communication/`

Plugin README documents a text command protocol (`PING`, `LED_ON`, etc.) for the generic sketches — separate from the Andy binary frame protocol used by the ship hub path.

Additional plugin doc: `Plugins/ArduinoCommunication/MULTI_DISPLAY_SETUP.md`.

---

## 9. Networking summary (what exists in code)

This is **not** a full custom netcode stack. Observed pieces:

1. **Unreal replication** — Mission GameState/PlayerState fields; thruster/turret/enemy health and squad fields marked `Replicated` / `ReplicatedUsing` where declared.
2. **Server-only hardware paths** — `UShipHardwareInputComponent` and `UMiniCRTWeaponDisplayComponent` expose `bServerOnly` (default true) and authority checks.
3. **UDP outbound** — `UMinimapUDPSenderComponent` for external minimap.
4. **Serial / TCP to hardware** — ArduinoCommunication (not UE multiplayer networking).
5. **Editor TCP MCP** — UnrealMCP bridge for tooling.

Module deps that enable sockets: game module private `Sockets`+`Networking`; Arduino plugin public `Sockets`+`Networking`.

---

## 10. Content / Blueprint organization

Under `Content/Blueprints/` (verified asset names):

| Area | Examples |
|------|----------|
| Ship / player | `BP_Hovercraft`, `PC_Controller`, `BP_PlayerState`, `GameMode_ShipPorts`, `BPC_ESP32Comm` |
| Weapons / world interactables | `BP_Turret` (now `AEnemyTurretPawn` enemy emplacement), `BP_Projectile`, `BP_TestShooter`, `BP_ResourceNode` |
| Interfaces | `BPI_Activatable`, `BPI_Beamable`, `BPI_Scannable`, `BPI_ESPComm`, `BPI_Missions`, `BPI_Projectile`, `BPI_DebugUI` |
| Level logic | `BP_Beamable`, `BP_Scannable`, `BP_DoorOpen`, `BP_ResourceNode` |
| Missions | Data assets + waypoint/destroyable + mission completer component + UI |
| Enemy AI | BT/BB/enums + example controllers/pawns + BP BT tasks |
| Camera / RT | `CameraLogic/*`, render-target UI widgets |
| UI | Mission objective widgets, Andy diag panel `WBP_AndyDiagPanel`, debug UI |
| Structs/enums (BP) | `Str_Enum/*` (mags, thrusters, damage/resource structs) |

Art / environment: `Content/Art/`, `Content/Fab/`, `Content/Fractals/`, `Content/Level/`, `Content/MSPresets/`.

Agents modifying Blueprints should prefer **targeted edits** and Unreal MCP inspection when available (see project rules).

---

## 11. Naming conventions (observed)

| Pattern | Examples |
|---------|----------|
| `U` / `A` / `F` / `E` Unreal prefixes | Standard |
| Components: `*Component` | `HoverMovementComponent`, `ShipHardwareInputComponent` |
| Subsystems: `*Subsystem` | `MissionManagerSubsystem`, `AndySerialSubsystem` |
| Data assets: `*DataAsset` / `DA_*` content | `MissionDataAsset`, `DA_MissionTest1` |
| Blueprints: `BP_`, `BPC_`, `BPI_`, `WBP_`, `BT_`, `BB_`, `PC_`, `UI_` | Content tree |
| BT nodes: `BTTask_` / `BTService_` / `BTDecorator_` | C++ and some BP tasks |
| Enums: `E*` in C++; `E_*` in Content EnemyAI | `EEnemyCombatState` vs `E_CombatState` |
| Categories in UPROPERTY | Domain pipes: `Arduino\|Serial`, `Hover Movement\|Thrust`, `Mission\|Server`, `Ship Hardware\|Events` |
| Ship / hardware IDs | `FName ShipId` matching `AndySerialSubsystem::AddPort` |
| Mission IDs | Stable `FName` (guide examples: `Main_*`, `Sub_*`, `Side_*`) |

Gameplay tags in `Config/DefaultGameplayTags.ini` include:

- `Gameplay.Resource.*` (Crystal, Metal, Plant)
- `Gameplay.Targets.*` (Destructible, Enemy, Player)
- `Gameplay.Weapons.Scannable`, `Gameplay.Weapons.Tractorable`

---

## 12. UnrealMCP (editor tooling)

- Editor module; TCP JSON command bridge (`UEpicUnrealMCPBridge` editor subsystem).
- Command areas under `Plugins/UnrealMCP/.../Commands/` include editor, Blueprint, and Blueprint graph helpers.
- Used so external agents can inspect/edit Unreal assets while the editor is running (see `.cursor/mcp.json` / project MCP setup).

---

## 13. Config highlights

| File | Relevant contents |
|------|-------------------|
| `DefaultEngine.ini` | Maps, GameMode, renderer (DX12, Substrate, virtual shadows, ray tracing flags, etc.) |
| `DefaultGame.ini` | Project ID; ArduinoSerialPort verbose diagnostics; Asset Manager primary asset scan entries (Map / PrimaryAssetLabel / GameFeatureData) |
| `DefaultGameplayTags.ini` | Gameplay tags listed above |
| `DefaultInput.ini` | Standard axis configs (Enhanced Input is a module dependency; ship input is largely hardware + BP) |

Note: `DefaultGame.ini` Asset Manager scan paths still reference `/Game/Maps` while levels live under `/Game/Level` — observed as-is; do not assume auto-discovery of mission/enemy Primary Data Assets from INI alone.

---

## 14. Related documentation (in-repo)

| Document | Audience |
|----------|----------|
| `MISSION_SYSTEM_GUIDE.md` | Mission authoring & wiring |
| `Plugins/ArduinoCommunication/README.md` | Generic Arduino plugin API / sketches |
| `Plugins/ArduinoCommunication/MULTI_DISPLAY_SETUP.md` | Multi-display setup |
| `.cursor/rules/00-project-core.mdc` | Engineering priorities for AI agents |
| This file | Architecture overview for agents |

---

## 15. Practical map for a new senior engineer / AI agent

1. **Ship pawn & controls:** Content `BP_Hovercraft` + C++ hover/firing components + plugin `ShipHardwareInputComponent` / Andy serial.
2. **Session loop:** `GameMode_ShipPorts` (default) / optional `AMissionGameModeBase` + mission data assets + `UMissionManagerSubsystem`.
3. **Combat AI:** `UEnemyDefinition` → spawn → `AEnemyAIController` BT + director threat scaling.
4. **Hardware path:** Register ports on `UAndySerialSubsystem` → bind ship component by `ShipId` → optional diag / MiniCRT.
5. **Do not invent parallel hardware stacks** — extend Andy/serial patterns already present.
6. **Compile after C++ changes** (project rule / CLAUDE guidance): Editor Compile or UBT for `Unduinocpp` Win64 Development.

---

## 16. Explicit non-claims

The following were **not** verified as implemented systems in this pass and should not be assumed:

- Full multiplayer listen-server / dedicated-server product architecture beyond replication markers on specific classes
- Complete ESP-NOW protocol documentation inside Unreal (ESP-NOW is referenced as hardware-side forwarding in MiniCRT/Andy comments; Unreal talks serial/TCP)
- That root `README.md` / `CLAUDE.md` engine version or “plugin-only project” framing matches the current gameplay-heavy codebase

When unsure, open the cited header or Content asset rather than relying on this summary alone.
