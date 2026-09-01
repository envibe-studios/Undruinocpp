# Current Direction

This document describes the current state of development.

Unlike the North Star, this document is expected to evolve frequently.

Its purpose is to rapidly onboard future AI engineers so they immediately understand what the team is currently trying to accomplish.

**How to read labels in this file**

| Label | Meaning |
|-------|---------|
| *(verified)* | Observed in source, configs, Content asset names, git history, or agent-session evidence |
| *(inferred)* | Reasonable conclusion from that evidence; not an authored milestone statement from the lead engineer |

Last populated: 2026-08-07.

---

# Current Milestone

**Enemy AI Content bring-up on top of a landed C++ framework**, while keeping the physical bridge (Andy / weapons / diagnostics) session-ready. *(inferred)*

**Verified context:** A full Enemy AI C++ stack exists under `Source/Unduinocpp/AI/` (pawn, controller, definition data assets, movement modes, abilities, aggro/health, spawner, threat director, squad coordinator, BT nodes). Content under `Content/Blueprints/EnemyAI/` includes test/example assets (`BP_AIEXample`, `AI_TestController`, `NewBlueprint`, BP BT tasks) alongside newer-looking flyer/definition assets. Agent work on 2026-08-05–07 focused on designing/implementing this AI system and wiring Content via Unreal MCP. Parallel that same week: agent onboarding docs (`PROJECT_ARCHITECTURE`, `ENGINEERING_PHILOSOPHY`, `ARCHITECTURAL_DECISIONS`).

Hardware-facing systems (Andy serial hub, ship hardware input, firing/mags/IMU, MiniCRT via `!crt`, Andy diagnostics) are treated as preserve-first cores in architecture docs. Git history through May 2026 shows heavy investment in hover, dual weapons, multi-display, reloader/mags. Recent uncommitted/working-tree signals include MiniCRT Blueprint wiring and `WBP_AndyDiagPanel`.

Mission *runtime* C++ and designer guide exist; Content includes test assets (`DA_MissionTest1/2`) plus a graybox integration expedition map `/Game/Level/GrayboxExpedition` with registry `/Game/Blueprints/Missions/Graybox/DA_MissionRegistry_Graybox` (`GameMode_GrayboxExpedition`).

---

# Immediate Priorities

Order is **inferred** from recent work signals and system maturity. Reorder if the lead engineer states otherwise.

1. **Finish Enemy AI Content wiring** — Behavior Trees / blackboard alignment with C++ keys, first data-driven archetypes (flyer + **stationary turret** via `BP_Turret` / `DA_EnemyDefinition_Turret`), spawn path on `EnemyTest` (and eventually ship maps). *(inferred priority; verified that C++ framework + WIP Content exist; turret emplacement path landed 2026-08-18)*
2. **Keep dual-weapon + MiniCRT + Andy diag session-ready** — Port/Starboard IMU/mag/reload routing, `!crt` displays, roll-call diagnostics UI. *(inferred polish priority; verified systems exist and Content was recently touched)*
3. **Playtest graybox expedition** — `/Game/Level/GrayboxExpedition` is an integration sandbox for hover travel, scan, harvest proxies, ambush, breach/destroy base, and mission fail-forward. Layout XY scaled ~10× (Hub Y=30k, Scan/Harvest ±80k, Ambush Y=90k, Base Y≈130–150k). Ground is heightmap mesh `/Game/Level/Graybox/SM_GrayboxHeightmapTerrain` (`GrayboxHeightmapTerrain`, always-loaded, complex/convex collision). Source PNG + generator: `Content/Level/Graybox/T_GrayboxHeightmap.png`, `Tools/heightmap_to_obj.ps1`. *(verified; smoke partially run)*
4. **Clarify default map / session entry** — `DefaultEngine.ini` GameDefaultMap points at `/Game/Level/TestLevel`; editor startup map is now `HoverHandlingCourse` (hover feel-tuning circuit). Confirm intended packaged/play entry vs editor tuning entry. *(verified config values; inferred that GameDefaultMap still needs a decision)*
5. **World/art exploration stays parallel, not blocking** — Fractals, CliffRock, Fab packs, Gaea/mesh-partition experiments. *(inferred priority ranking; verified Content/plugins present)*

---

# Systems Considered Stable

Modify carefully. Preserve contracts and compatibility. Stable does **not** mean bug-free.

| System | Notes |
|--------|--------|
| **Andy serial hub** (`UAndySerialSubsystem`) | Per-`ShipId` GameInstance port ownership; production ship uplink. *(verified)* |
| **Binary frame parser** (`UByteStreamPacketParser`) | `0xAA…0x55` frames + CRC; shares UART with ASCII. *(verified)* |
| **Ship hardware binding** (`UShipHardwareInputComponent`) | Typed events, Port/Starboard IMU, mag apply, `bServerOnly` default. *(verified)* |
| **Andy diagnostics** (`UAndyDiagSubsystem` + registry + roll call) | `!diag,quick` / `DIAG_*`; default 10-node inventory. *(verified)* |
| **Hover vehicle cores** (`UHoverMovementComponent`, `UHoverThrusterComponent`) | Analog-first; thruster health/malfunctions. *(verified)* |
| **Firing / weapon mags** (`UFiringComponent`, `UWeaponMag*`) | Bullet/Tractor/Scanner/Custom; IMU + RFID mag path; tractor is trace + delegates only in C++. *(verified)* |
| **Mission runtime C++** | Data assets + `UMissionManagerSubsystem` + Mission GameState/PlayerState; fail-forward documented. *(verified framework; Content still thin)* |
| **Generic Arduino Serial/TCP component** | Intentionally retained alongside Andy. *(verified decision)* |
| **Ship Content shell** | `GameMode_ShipPorts`, `BP_Hovercraft`, `PC_Controller`, `BPC_ESP32Comm` as the default session assembly. *(verified as default GameMode / primary ship assets)* |

---

# Systems Under Active Development

Expected to change significantly.

| System | Signal |
|--------|--------|
| **Enemy AI Content + BT wiring** | Large C++ tree landed; Content still includes example/test names and BP tasks alongside C++ nodes. *(verified)* |
| **Squad synergy / teamwork** | `ASquadCoordinator` header explicitly marks “Future teamwork / synergy layer.” *(verified)* |
| **MiniCRT + diag UI Content** | `UMiniCRTWeaponDisplayComponent` + BP; `WBP_AndyDiagPanel`; MiniCRT log category noted “used while testing.” *(verified)* |
| **Mission content / session integration** | Test mission DAs exist; whether `GameMode_ShipPorts` fully wires the mission registry in every session is **not verified** here — treat as open. *(inferred gap)* |
| **Threat / AI director tuning** | Director-lite scales aggression/cooldowns from mission threat. *(verified code; tuning inferred as unfinished)* |
| **Environment / art tech** | Fractals showcase, rock packs, Gaea/mesh-partition maps. *(verified presence; role inferred as look-dev)* |
| **Agent tooling & docs** | UnrealMCP + `.cursor/docs` onboarding sprint. *(verified)* |

---

# Current Technical Debt

Meaningful architectural debt only.

| Debt | Why it exists / when to revisit |
|------|----------------------------------|
| **Plugin ↔ game module coupling** | `ArduinoCommunication` publicly depends on `Unduinocpp` (mags/firing types); MiniCRT lives in game module but sends via plugin serial. Accepted for product speed. Revisit only with a migration plan — not purity cleanup. *(verified)* |
| **Dual protocols on one Andy UART** | Binary telemetry + ASCII `!diag` / `!crt`. Consumers must filter. Revisit only if a second cable or protocol split is explicitly chosen. *(verified)* |
| **Dual Arduino APIs** | Generic Serial/TCP component + Andy hub. Useful for demos; confuses onboarding. Keep both until a deliberate retirement. *(verified)* |
| **Legacy vs fuller diag UI** | Older two-node layouts can coexist with roll-call UI. Prefer dual bindings over forced rebuilds. *(verified from decisions doc)* |
| **Stale root docs** | `CLAUDE.md` / `README` still mention UE 5.7; project targets **5.8**. Prefer `.uproject` + `PROJECT_ARCHITECTURE.md`. *(verified)* |
| **Asset Manager path mismatch** | `DefaultGame.ini` still scans `/Game/Maps` while levels live under `/Game/Level`. Do not assume auto-discovery of mission/enemy Primary Data Assets from INI alone. *(verified)* |
| **Placeholder C++ stubs** | `MyClass` / `MyClass2` are unused stubs. Safe to ignore or delete when convenient. *(verified)* |
| **ScreenBridge plugin stub** | Marketplace multi-window plugin present; module startup is empty in checkout; 5.8 upgrade historically painful. Prefer project `UMultiDisplayCameraComponent` for bridge monitors unless ScreenBridge is intentionally revived. *(verified architecture + inferred preference)* |
| **BP enum mirrors of C++ AI enums** | Content `E_*` assets can drift from `EEnemy*` sources of truth. Revisit when AI Content stabilizes. *(verified risk)* |

---

# Active Experiments

Intentionally unstable. Do not treat as production contracts.

- **EnemyTest map + example AI Blueprints** (`BP_AIEXample`, `AI_TestController`, `NewBlueprint`, BP chase/roam tasks). *(verified)*
- **Squad Carry/Drop synergy** — code exists; header frames it as future layer. *(verified)*
- **MiniCRT bring-up instrumentation** — dedicated log category while testing. *(verified)*
- **Art / world look-dev** — Fractals Showcase, CliffRockPack, Fab low-poly packs, `GaeaCanyons`, `TestMeshTerrain`, mesh-partition plugins. *(verified)*
- **External minimap UDP** (`UMinimapUDPSenderComponent`) — side channel separate from Andy. *(verified)*
- **Tractor/scanner presentation** — still content-side. Target reactions use `BPI_Beamable` / `BPI_Scannable`. *(verified)*

---

# Recently Learned Lessons

Future AI engineers should not rediscover these.

1. **Extend Andy — do not open parallel COM ports** for ship devices (including MiniCRT). Route through `UAndySerialSubsystem` by `ShipId`. *(verified decision)*
2. **ESP-NOW is hardware-side**; Unreal speaks serial/TCP. Leaf wireless issues need firmware tooling. *(verified)*
3. **Shared UART means filter ownership** — binary frames and ASCII lines coexist; diag consumers should only eat `DIAG_*` (etc.). *(verified)*
4. **Wrong `ShipId` looks like dead hardware** when the port is fine. *(verified tradeoff)*
5. **IMU calibration complexity can break aiming** — git history shows a quaternion zeroing approach was reverted after it broke weapon aim; prefer careful, tested orientation changes. *(verified from commits)*
6. **Tractor side effects belong outside C++ firing core** — physics pull was removed from `UFiringComponent`; keep content-specific effects on delegates. *(verified from commits + headers)*
7. **Multi-display windows are finicky** — many sequential fixes for blank secondary displays / Slate volatility / capture timing. Touch `UMultiDisplayCameraComponent` carefully; prefer Standalone Game for installation. *(verified from commits + setup docs)*
8. **Engine version truth is 5.8** from `.uproject` / Target / plugin descriptors — ignore stale 5.7 mentions in older markdown. *(verified)*
9. **Optional session wiring** — mission system can use `AMissionGameModeBase` *or* manual registry registration; forgetting wiring means missions silently never start. *(verified guide)*
10. **Preserve diagnostics** — do not delete diag paths to reduce log noise; tune filters. *(verified project rules)*

---

# Known Risks

| Risk | Notes |
|------|--------|
| Andy is a single point of failure per ship | Entire hardware path for that ship. *(verified tradeoff)* |
| COM port must live on the authority machine | Hardware paths default `bServerOnly`; multi-machine setups need a clear owner. *(verified)* |
| Full multiplayer product architecture not verified | Replication exists on selected classes; do not assume listen/dedicated readiness. *(verified non-claim)* |
| Threat tuning can suddenly harden the room | AI director listens to mission threat. *(verified)* |
| MiniCRT can desync if sends drop | Change-driven + debounce, not a full ack protocol in Unreal. *(verified)* |
| Framing bugs mimic diag flakiness | Shared UART. *(verified)* |
| Generic TCP sketches mislead newcomers | Ship architecture is Andy serial + ESP-NOW leaves, not Unreal↔each-node WiFi. *(verified)* |
| Default map may be wrong/missing | Config GameDefaultMap = `TestLevel`; confirm asset presence and intended entry map before assuming PIE defaults work. *(verified config; map presence not re-verified in this pass)* |
| Plugin upgrade fragility | ScreenBridge / engine moves have caused compile and open failures historically. *(verified from agent history)* |
| Engineering/repair gameplay depth unclear | `RepairProgress` exists as `EEspMsgType`; how deep station gameplay goes beyond the packet is **inferred** as underdeveloped. |

---

# Definition of Done

**Current milestone is complete when** *(inferred criteria — confirm with lead engineer)*:

1. `EnemyTest` (or equivalent) can spawn at least one data-driven enemy archetype from `UEnemyDefinition` with a working Behavior Tree / blackboard.
2. Mission threat changes visibly affect AI via `UEnemyAIDirectorSubsystem` (aggression / cooldown scaling) in a playable test.
3. Ship hardware path still brings up Port/Starboard weapons, MiniCRT ammo display, and Andy roll-call diagnostics without regressions on the intended session map.
4. No new parallel hardware, mission, or AI stacks introduced during the bring-up.

---

# Questions Currently Being Explored

These are open design questions, not bugs.

1. **How far to push squad synergy now vs later?** Coordinator exists but is labeled a future teamwork layer. *(verified tension)*
2. **Is mission registration wired into `GameMode_ShipPorts` for real sessions, or only via optional Mission GameMode?** Needs editor/Content verification. *(inferred open question)*
3. **What is the first production mission beyond `DA_MissionTest*`?** Content volume and narrative beat not settled. *(inferred)*
4. **How deep should engineering / repair / jack gameplay go** beyond existing ESP message types? *(inferred)*
5. **Threat scaling feel** — director-lite knobs vs a richer pacing director later? *(inferred from “Director-lite” naming)*
6. **Which map is the canonical session entry** (`HoverTrack` vs ship maps vs `EnemyTest` vs default `TestLevel`)? *(inferred from config split)*
7. **Should ScreenBridge be revived, or is `UMultiDisplayCameraComponent` the long-term bridge monitor path?** *(inferred)*

Future discussions should begin here instead of restarting these conversations.

---

# Notes for Future AI Engineers

When beginning work:

1. Read NORTH_STAR.md
2. Read PROJECT_ARCHITECTURE.md
3. Read ENGINEERING_PHILOSOPHY.md
4. Read ARCHITECTURAL_DECISIONS.md
5. Read this document.
6. Read any relevant playbooks before implementation.

Understand the current direction before proposing major changes.

If implementation conflicts with the North Star, discuss it before proceeding.

**Working defaults for this phase**

- Prefer extending Enemy AI definitions / BT / Content over rewriting the AI framework.
- Prefer extending Andy / ShipHardwareInput / MiniCRT / diag over new I/O stacks.
- Prefer mission data assets + threat hooks over one-off spawn timers for session pressure.
- Label speculation as inference when updating this file again.
)

## Current Development Philosophy

The current objective is not to build the final game.

The objective is to discover what is genuinely fun.

A graybox vertical slice is more valuable than an unfinished masterpiece.

Temporary implementations are acceptable when they accelerate learning, provided they do not permanently compromise the architecture.

Art polish, world scale, narrative depth, and content volume should follow proven gameplay rather than precede it.

## Current Success Criteria

Before expanding the project, the following questions should have confident answers:

- Is driving fun?
- Is engineering fun?
- Is combat fun?
- Is exploration fun?
- Is the crew communication fun?
- Are missions memorable?
- Is the pacing enjoyable?
- Does hardware improve immersion?

Only after these questions are answered should major content expansion begin.